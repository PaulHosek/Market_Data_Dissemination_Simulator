#include <iostream>

#include "generator/MarketDataGenerator.h"
#include "generator/types.h"
#include "chrono"

// TIP To <b>Run</b> code, press <shortcut actionId="Run"/> or click the <icon src="AllIcons.Actions.Execute"/> icon in the gutter.
int main() {
    // using namespace types;
    using namespace std::chrono_literals;
    // QueueType_Quote x{};
    // auto i = x;
    types::QueueType_Quote qq{};
    types::QueueType_Trade tq{};
    MarketDataGenerator mdg(qq, tq);

    mdg.configure(1000, "../data/tickers.txt");
    mdg.start();

    mdg.stop();





}




// // TIP Press <shortcut actionId="RenameElement"/> when your caret is at the <b>lang</b> variable name to see how CLion can help you rename it.
// auto lang = "C++";
// std::cout << "Hello and welcome to " << lang << "!\n";
//
//
// for (int i = 1; i <= 5; i++) {
//     // TIP Press <shortcut actionId="Debug"/> to start debugging your code. We have set one <icon src="AllIcons.Debugger.Db_set_breakpoint"/> breakpoint for you, but you can always add more by pressing <shortcut actionId="ToggleLineBreakpoint"/>.
//     std::cout << "i = " << i << std::endl;
// }
//
// return 0;
// // TIP See CLion help at <a href="https://www.jetbrains.com/help/clion/">jetbrains.com/help/clion/</a>. Also, you can try interactive lessons for CLion by selecting 'Help | Learn IDE Features' from the main menu.