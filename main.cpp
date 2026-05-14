#include "MyForm.h"

using namespace System;
using namespace System::Windows::Forms;

[STAThreadAttribute]
int main(array<String^>^)
{
    Application::EnableVisualStyles();
    Application::SetCompatibleTextRenderingDefault(false);

    CMLab4::MyForm form;
    Application::Run(% form);
    return 0;
}