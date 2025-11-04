#include <wx/wx.h>          
#include "Main.h"

// 应用程序类
class MyApp : public wxApp {
public:
    virtual bool OnInit() override {
        wxInitAllImageHandlers();
        MainFrame* frame = new MainFrame();
        frame->Show(true);
        return true;
    }
};
wxIMPLEMENT_APP(MyApp);
