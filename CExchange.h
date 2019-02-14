#pragma once
#include<windows.h>
#include<string>
#include<queue>
#include<map>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <regex>
using namespace std;
 
 
namespace Trading_simulation
{
	
	class Order
	{ 
	public:
		Order(void)
		{
			//If the number of orders is huge, might consider using a boost memory pool.
			pnQuantity = new int;
			*pnQuantity = 0;
			fLimitPrice = 0;
			uSerialNo = 0;
		}

		//for deep copy
		Order(const Order &order)
		{ 
			sBuyerSeller = order.sBuyerSeller;
			sInstrument = order.sInstrument;
			pnQuantity = new int;
			*pnQuantity = *order.pnQuantity;

			fLimitPrice = order.fLimitPrice;
			uSerialNo = order.uSerialNo;
 		}
		~Order(void)
		{
			if (pnQuantity)
			{ 
				delete pnQuantity;
				pnQuantity = NULL;
			}
			 
		}

		//Order sorting rule
		bool operator < (const Order &order) const
		{
			//if (order.fLimitPrice == fLimitPrice)
			//Generally, using precision instead of = to check float numbers
			if (fabs(order.fLimitPrice - fLimitPrice) < 0.0000001)
				return order.uSerialNo < uSerialNo;
			else
			    return *pnQuantity > 0 ? order.fLimitPrice > fLimitPrice :order.fLimitPrice < fLimitPrice;
		}
		 
		 
		Order & operator=(const Order &order) 
		{
			if (this != &order)
			{
				sBuyerSeller = order.sBuyerSeller;
				sInstrument = order.sInstrument;
			   // pnQuantity = new int;
				*pnQuantity = *order.pnQuantity;
 
				fLimitPrice = order.fLimitPrice;
				uSerialNo = order.uSerialNo;
 			}
			return *this;
		} 

		bool Parse(vector<string> &items)
		{
			bool ret = false;

			do
			{
				if (items.size() != 4)
					break;

				*pnQuantity = stoi(items[2]);
				if (*pnQuantity == 0)
					break;
 
				fLimitPrice = stod(items[3]);
				if (fLimitPrice == 0)
					break;

				sBuyerSeller = items[0];
				sInstrument = items[1];

				ret = true;
			} while (false);

			return ret;
		}
		string       ToString()
		{
			std::ostringstream oss;
			oss << std::noshowpoint << fLimitPrice;
			std::string str = oss.str();

			return sBuyerSeller + ":"  + sInstrument + ":" + to_string(*pnQuantity) + ":" + str;
		}
	 
	public:
		string       sBuyerSeller;
		string       sInstrument;

		//Using the int pointer, we can directly modify the order's quantity field in the priority_queue, which is efficient.
		int          *pnQuantity;

		double       fLimitPrice;

		//Order serial number, together with the price, determine the priority when matching
		unsigned int uSerialNo;
	};

	//Buy and sell orders for each instrument
	class OrderPair
	{
		public:
			priority_queue<Order>   buy;
			priority_queue<Order>   sell;
	};

	class Trade
	{
	public:
		string       ToString()
		{  
			std::ostringstream oss;
			oss <<  std::noshowpoint << fLimitPrice;
			std::string str = oss.str();
			 
			return sBuyer + ":" + sSeller + ":" + sInstrument + ":" + to_string(nQuantity) + ":" +str;
		}
	public:
		string       sBuyer;
		string       sSeller;
		string       sInstrument;
		int          nQuantity;
		double       fLimitPrice;
		 
	};
	typedef map<string, OrderPair>           ExchangeMap;
	typedef map<string, OrderPair>::iterator ExchangeMapIterator;

	class CExchange
	{
	public:
		CExchange()
		{
			m_serialNumber = 0;
		}
 
		~CExchange()
		{
			 m_Orders.clear();
 			 
 			 m_buyOrdersSequence.clear();
 			 m_allTraders.clear();
		}

		size_t  AddOrders(vector<Order> &orders)
		{
			m_Orders.clear();
 			m_buyOrdersSequence.clear();
			m_allTraders.clear();

			ExchangeMapIterator itr;
			for(size_t i=0;i<orders.size();i++)
			{
				InterlockedIncrement(&m_serialNumber);
				orders[i].uSerialNo = m_serialNumber;

				itr = m_Orders.find(orders[i].sInstrument);

				if (itr == m_Orders.end())
				{
					OrderPair op;

					if (*orders[i].pnQuantity > 0)
					{
						op.buy.push(orders[i]);
					}
					else
					{
						op.sell.push(orders[i]);
					}

					m_Orders[orders[i].sInstrument] = op;

					m_buyOrdersSequence.push_back(orders[i].sInstrument);
				}
				else
				{
					if (*orders[i].pnQuantity > 0)
					{
						itr->second.buy.push(orders[i]);
					}
					else
					{
						itr->second.sell.push(orders[i]);
					}
				}

			}
			 
			return m_Orders.size();
		}

		size_t  MatchOrders()
		{
			size_t count = 0;

			ExchangeMapIterator itr;
		
			for (size_t i = 0; i < m_buyOrdersSequence.size(); i++)
			{
				itr = m_Orders.find(m_buyOrdersSequence[i]);
			    if(itr != m_Orders.end())
				{ 
				 	count += MatchInstrument(itr->first, itr->second.buy, itr->second.sell);
 				}

			}
			

			return count;
		}

		size_t ExportTradeRecords(string filename)
		{
			ofstream   out(filename, ios::trunc);
			if (!out)
				return 0;
 
			for (size_t i = 0; i < m_allTraders.size(); i++)
			{
				out <<m_allTraders[i].ToString() << endl;
			}
			
			out.close();

			return m_allTraders.size();
		}
		void PrintRemainingOrders()
		{ 
			cout <<  endl<<"Remaining orders" <<  endl << endl;;
			Order o;
			ExchangeMapIterator itr = m_Orders.begin();
		
			while(itr!=m_Orders.end())
			{ 
				if (!itr->second.buy.empty() || !itr->second.sell.empty())
				{

					cout << endl << itr->first << endl;
					
					cout << "------------------" << endl;
					if (!itr->second.buy.empty())
					{
						cout << "Buy:" << endl;
						while (!itr->second.buy.empty())
						{
							o = itr->second.buy.top();
							itr->second.buy.pop();

							cout << o.ToString() << endl;
						}
					}
					if (!itr->second.sell.empty())
					{
						cout << "Sell:" << endl;
						while (!itr->second.sell.empty())
						{
							o = itr->second.sell.top();
							itr->second.sell.pop();

							cout << o.ToString() << endl;
						}
					}
					
					cout << endl;
				}
				 
				++itr;
			}
		 
		}
	private:
		size_t  MatchInstrument(string sInstrument,priority_queue<Order> &buyers, priority_queue<Order> &sellers)
		{
			if (buyers.empty() || sellers.empty())
				return 0;

			size_t count = 0;

			Trade td;

			bool matched = false;

			while (true)
			{
				matched = false;
 
#define TopBuyer   buyers.top()
#define TopSeller  sellers.top()

				 
				//Buyers place orders first than sellers
				if (TopBuyer.uSerialNo < TopSeller.uSerialNo)
				{
					if (TopBuyer.fLimitPrice >= TopSeller.fLimitPrice)
					{
						matched = true;

						td.sInstrument = sInstrument;
						td.fLimitPrice = TopBuyer.fLimitPrice;
						td.sBuyer = TopBuyer.sBuyerSeller;
						td.sSeller = TopSeller.sBuyerSeller;
						 
						//normally, you must first pop, modify and then push again. 
						//Pointer can increase efficiency, but pointer can also reduce the readability and increase risk . 
						//You need to consider whether you must use it

						if (*TopBuyer.pnQuantity + *TopSeller.pnQuantity == 0)
						{
							td.nQuantity = *TopBuyer.pnQuantity;

							buyers.pop();
							sellers.pop();
						}
						else if (*TopBuyer.pnQuantity + *TopSeller.pnQuantity > 0)
						{
							*TopBuyer.pnQuantity += *TopSeller.pnQuantity;
							 
							td.nQuantity = *TopSeller.pnQuantity*-1;

							sellers.pop();
						}
						else
						{
							*TopSeller.pnQuantity+= *TopBuyer.pnQuantity;
							 
							td.nQuantity = *TopBuyer.pnQuantity;

							buyers.pop();
						}
 
					}
				}
				else  //Sellers place orders first than buyers
				{
					if (TopSeller.fLimitPrice <= TopBuyer.fLimitPrice)
					{
						matched = true;

						td.sInstrument = sInstrument;
						td.fLimitPrice = TopSeller.fLimitPrice;
						td.sBuyer = TopBuyer.sBuyerSeller;
						td.sSeller = TopSeller.sBuyerSeller;

						if (*TopSeller.pnQuantity + *TopBuyer.pnQuantity == 0)
						{
							td.nQuantity = *TopBuyer.pnQuantity;

							buyers.pop();
							sellers.pop();
						}
						else if (*TopSeller.pnQuantity + *TopBuyer.pnQuantity > 0)
						{
							*TopBuyer.pnQuantity += *TopSeller.pnQuantity;
						 
							td.nQuantity = *TopSeller.pnQuantity*-1;

							sellers.pop();
						}
						else
						{
							*TopSeller.pnQuantity+= *TopBuyer.pnQuantity;
							 
							td.nQuantity = *TopBuyer.pnQuantity;

							buyers.pop();
						}
					}

					
				}
			
				if (!matched)
					break;

				m_allTraders.push_back(td);
				count++;

				if ( buyers.empty() || sellers.empty())
					break;
			}
             
			return count;
		}

		 
	private:
		ExchangeMap    m_Orders;
 

		//Keep buy instruments trading order
		vector<string> m_buyOrdersSequence;

		unsigned int   m_serialNumber;

		vector<Trade>  m_allTraders;
	};


	namespace Utility
	{ 
		vector<string> splitLine(const string& in, const string& delim)
		{
			regex re{ delim };
			return vector<string> { sregex_token_iterator(in.begin(), in.end(), re, -1), sregex_token_iterator() };


		}

		size_t splitLine2(const string& in, const string& delim, vector<string> &strings)
		{ 
			strings.clear();
			istringstream f(in);
			string s;
			while (getline(f, s, ':'))
			{
					strings.push_back(s);
			}
 			return strings.size();
		}
		

		template<typename T>
		vector<T>   ReadOrdersFromFile(string filename)
		{
			vector<T> orders;
			T order;
			try
			{ 
				ifstream in(filename);
				string filename;
				string line;
				vector<string> strings;

				if (in)
				{ 
					while (getline(in, line))  
					{ 
						if (splitLine2(line, ":", strings) == 4)
						{
							if (order.Parse(strings))
							{
								orders.push_back(order);
							}
							else
							{
								cout << line + " not valid" << endl;
							}
						}
						

					}
				}
				else 
				{
					cout << "no such file" << endl;
				}

			}
			catch (const std::exception& e)
			{
				cout << "error reading orders:" << e.what() << endl;

				orders.clear(); 
			}

			return orders;;
		}
	}
 
 
	
}


