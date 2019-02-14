# Trading_simulation
The most basic securities foreign exchange trading rules, simulation and implementation


  vector<Order> orders=Utility::ReadOrdersFromFile<Order>(fileOrders);

	std::cout <<"Read "<< orders.size() << " order records\n\n" ;

	DWORD tickct=GetTickCount();

	std::cout << "Start matching\n";

	CExchange exchange;
	exchange.AddOrders(orders);
	exchange.MatchOrders();
	size_t ret=exchange.ExportTradeRecords(fileTrades);
 
	std::cout <<"End matching, "<< ret << " trade(s),It takes " << GetTickCount()-tickct << " milliseconds\n";

	std::cout << "\nPress any key to print remaining orders";
	cin.get();

	exchange.PrintRemainingOrders();
