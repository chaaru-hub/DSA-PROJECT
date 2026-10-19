// ============================================================================
// main.cpp - entry point
//
//   TreeViz                      normal interactive program
//   TreeViz --light              start with the light theme (projectors)
//   TreeViz --selftest           run thousands of random insert / delete /
//                                search operations and check the BST property
//                                after each one (no window needed)
//   TreeViz --autotest           drive the real main loop with simulated input
//   TreeViz --shot out.bmp "Insert:50 30 70|Search:30" [frame t]
//                                run button presses and save a screenshot
// ============================================================================
#include <SDL.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>
#include <string>
#include "App.h"

static int runSelfTest() {
    BSTVis bst;
    bst.init();
    std::mt19937 rng(1000);
    std::string err = bst.selfTest(rng, 5000);
    if (err.empty()) err = bst.validate();
    std::printf("[%s] %s %s\n", err.empty() ? " ok " : "FAIL", bst.title().c_str(), err.c_str());
    return err.empty() ? 0 : 1;
}

int main(int argc, char* argv[]) {
    if (argc >= 2 && std::strcmp(argv[1], "--selftest") == 0) return runSelfTest();
    for (int i = 1; i < argc; ++i)
        if (std::strcmp(argv[i], "--light") == 0) toggleTheme();   // start in the light theme

#ifdef __EMSCRIPTEN__
    static App app;     // must outlive main(): the browser keeps calling app.frame()
#else
    App app;
#endif
    if (!app.init()) return 1;
    if (argc >= 3 && std::strcmp(argv[1], "--shot") == 0) {
        std::string script = argc >= 4 ? argv[3] : "";
        if (script == "--light") script.clear();
        int frame = argc >= 5 ? std::atoi(argv[4]) : -1;
        float t = argc >= 6 ? (float)std::atof(argv[5]) : 1.0f;
        bool ok = app.screenshot(argv[2], script, frame, t);
        app.shutdown();
        return ok ? 0 : 1;
    }
    app.run(argc >= 2 && std::strcmp(argv[1], "--autotest") == 0);
    app.shutdown();
    return 0;
}
