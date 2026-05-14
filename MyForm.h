#pragma once
#include "Solver.h"
#include <sstream>
#include <iomanip>

namespace CMLab4 {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;
    using namespace System::Drawing::Drawing2D;
	using namespace ZedGraph;

    namespace SWF = System::Windows::Forms;

    // ================================================================
    //  SurfacePlot — heatmap-визуализация сеточной функции на ZedGraph.
    //  Каждая ячейка сетки — BoxObj в координатах (x, y); цвет — z(x,y).
    //  Для крупных сеток применяется подвыборка (≈ до 60×60 ячеек).
    // ================================================================
    ref class SurfacePlot : public Panel
    {
    private:
        ZedGraphControl^ zgc;

        static Color ColorMap(double t) {
            int r, gr, bl;
            if (t < 0.25)      { r = 0; gr = (int)(t * 4 * 100);
                                 bl = (int)(100 + t * 4 * 155); }
            else if (t < 0.5)  { r = 0; gr = (int)(100 + (t - 0.25) * 4 * 155);
                                 bl = (int)(255 - (t - 0.25) * 4 * 255); }
            else if (t < 0.75) { r = (int)((t - 0.5) * 4 * 255);
                                 gr = 255; bl = 0; }
            else               { r = 255; gr = (int)(255 - (t - 0.75) * 4 * 255);
                                 bl = 0; }
            return Color::FromArgb(
                std::min(255, std::max(0, r)),
                std::min(255, std::max(0, gr)),
                std::min(255, std::max(0, bl)));
        }

    public:
        SurfacePlot() {
            this->BackColor = Color::FromArgb(18, 20, 30);

            zgc = gcnew ZedGraphControl();
            zgc->Dock = DockStyle::Fill;
            zgc->IsShowPointValues = false;
            zgc->IsEnableHPan = false;
            zgc->IsEnableVPan = false;
            zgc->IsEnableHZoom = true;
            zgc->IsEnableVZoom = true;
            this->Controls->Add(zgc);

            GraphPane^ pane = zgc->GraphPane;
            pane->Fill = gcnew Fill(Color::FromArgb(18, 20, 30));
            pane->Chart->Fill = gcnew Fill(Color::FromArgb(14, 16, 26));
            pane->Border->Color = Color::FromArgb(80, 80, 110);
            pane->Chart->Border->Color = Color::FromArgb(80, 80, 110);

            pane->Title->FontSpec->FontColor = Color::White;
            pane->Title->FontSpec->Family = "Consolas";
            pane->Title->FontSpec->Size = 11;
            pane->Title->FontSpec->IsBold = true;
            pane->Title->FontSpec->Fill = gcnew Fill(Color::Transparent);
            pane->Title->FontSpec->Border->IsVisible = false;

            pane->XAxis->Title->Text = "x";
            pane->YAxis->Title->Text = "y";
            for each (Axis ^ ax in gcnew array<Axis^>{pane->XAxis, pane->YAxis}) {
                ax->Title->FontSpec->FontColor = Color::FromArgb(180, 200, 255);
                ax->Title->FontSpec->Family = "Consolas";
                ax->Title->FontSpec->Size = 10;
                ax->Title->FontSpec->Fill = gcnew Fill(Color::Transparent);
                ax->Title->FontSpec->Border->IsVisible = false;
                ax->Scale->FontSpec->FontColor = Color::FromArgb(170, 190, 230);
                ax->Scale->FontSpec->Family = "Consolas";
                ax->Scale->FontSpec->Size = 8;
                ax->Color = Color::FromArgb(120, 130, 170);
                ax->MajorTic->Color = Color::FromArgb(120, 130, 170);
                ax->MinorTic->Color = Color::FromArgb(120, 130, 170);
                ax->MajorGrid->IsVisible = false;
                ax->MinorGrid->IsVisible = false;
            }
            pane->Legend->IsVisible = false;
            pane->Margin->All = 8;
        }

        // Передать новый набор данных в график.
        void SetData(std::vector<double>* data, int n, int m,
                     double a, double b, double c, double d, String^ title)
        {
            GraphPane^ pane = zgc->GraphPane;
            pane->Title->Text = title;
            pane->CurveList->Clear();
            pane->GraphObjList->Clear();

            if (!data || data->empty() || n <= 0 || m <= 0) {
                zgc->AxisChange();
                zgc->Invalidate();
                return;
            }

            int Ny = m + 1;

            double zMin = (*data)[0], zMax = (*data)[0];
            for (size_t k = 1; k < data->size(); ++k) {
                double z = (*data)[k];
                if (z < zMin) zMin = z;
                if (z > zMax) zMax = z;
            }
            double range = zMax - zMin;
            if (range < 1e-15) range = 1.0;

            const int target = 60;
            int stepX = std::max(1, n / target);
            int stepY = std::max(1, m / target);

            double hx = (b - a) * (double)stepX / n;
            double hy = (d - c) * (double)stepY / m;

            for (int i = 0; i + stepX <= n; i += stepX) {
                for (int j = 0; j + stepY <= m; j += stepY) {
                    double z = (*data)[i * Ny + j];
                    double t = (z - zMin) / range;
                    Color col = ColorMap(t);
                    double x0 = a + (double)i / n * (b - a);
                    double y0 = c + (double)j / m * (d - c);
                    // BoxObj(x_left, y_top, width, height) — y_top = y0+hy
                    BoxObj^ box = gcnew BoxObj(x0, y0 + hy, hx, hy, col, col);
                    box->Border->IsVisible = false;
                    box->IsClippedToChartRect = true;
                    box->ZOrder = ZOrder::F_BehindGrid;
                    pane->GraphObjList->Add(box);
                }
            }

            String^ rangeText = String::Format("min={0:F4}   max={1:F4}", zMin, zMax);
            TextObj^ txt = gcnew TextObj(rangeText, 0.02, 0.02,
                CoordType::ChartFraction, AlignH::Left, AlignV::Top);
            txt->FontSpec->FontColor = Color::FromArgb(190, 210, 255);
            txt->FontSpec->Family = "Consolas";
            txt->FontSpec->Size = 8;
            txt->FontSpec->Fill = gcnew Fill(Color::FromArgb(0, 0, 0, 180));
            txt->FontSpec->Border->IsVisible = false;
            pane->GraphObjList->Add(txt);

            pane->XAxis->Scale->Min = a;
            pane->XAxis->Scale->Max = b;
            pane->YAxis->Scale->Min = c;
            pane->YAxis->Scale->Max = d;

            zgc->AxisChange();
            zgc->Invalidate();
        }
    };

	/// <summary>
	/// Summary for MyForm
	/// </summary>
	public ref class MyForm : public System::Windows::Forms::Form
	{
    public:
        MyForm() { InitializeComponent(); }

    private:
        // ---------- UI Controls ----------
        TabControl^ tabControl;

        // Вкладка «Параметры»
        GroupBox^ grpMethod;
        RadioButton^ rbSOR;
        RadioButton^ rbSeidel;
        NumericUpDown^ nudN, ^ nudM;
        NumericUpDown^ nudOmega;
        CheckBox^ chkAutoOmega;
        NumericUpDown^ nudEpsMet;
        NumericUpDown^ nudNmax;
        Button^ btnRunTest;
        Button^ btnRunMain;
        Button^ btnRunBoth;

        // Вкладка «Тестовая задача»
        RichTextBox^ rtbTest;
        Panel^ pnlTestGraphs;
        SurfacePlot^ sfExact, ^ sfInitTest, ^ sfNumTest, ^ sfDiffTest;

        // Вкладка «Основная задача»
        RichTextBox^ rtbMain;
        Panel^ pnlMainGraphs;
        SurfacePlot^ sfInitMain, ^ sfMain, ^ sfDiffMain;
        SurfacePlot^ sfInitMain2, ^ sfMain2;

        // Данные
        SolverResult* resTest = nullptr;
        SolverResult* resMain = nullptr;
        SolverResult* resMain2 = nullptr;  // удвоенная сетка
        std::vector<double>* diffTestData = nullptr;
        std::vector<double>* diffMainData = nullptr;

        // ============================================================
        //  Инициализация компонентов
        // ============================================================
        void InitializeComponent()
        {
            this->Text = L"Решение задачи Дирихле для уравнения Пуассона  |  Вариант 1";
            this->Size = Drawing::Size(1100, 780);
            this->MinimumSize = Drawing::Size(900, 650);
            this->BackColor = Color::FromArgb(18, 20, 30);
            this->ForeColor = Color::FromArgb(200, 210, 255);
            this->Font = gcnew Drawing::Font("Consolas", 9);

            tabControl = gcnew TabControl();
            tabControl->Dock = DockStyle::Fill;
            StyleTab(tabControl);

            // --- Вкладки ---
            auto tabParams = gcnew TabPage("  Параметры  ");
            auto tabTest = gcnew TabPage("  Тестовая задача  ");
            auto tabMain = gcnew TabPage("  Основная задача  ");
            for each (TabPage ^ tp in gcnew array<TabPage^>{tabParams, tabTest, tabMain})
                StyleTabPage(tp);

            tabControl->TabPages->AddRange(
                gcnew array<TabPage^>{tabParams, tabTest, tabMain});

            BuildParamsTab(tabParams);
            BuildTestTab(tabTest);
            BuildMainTab(tabMain);

            this->Controls->Add(tabControl);
        }

        SWF::Label^ addLabel(String^ text, int& y) {
            SWF::Label^ lbl = gcnew SWF::Label();
            lbl->Text = text;
            lbl->ForeColor = Color::FromArgb(140, 160, 220);
            lbl->Font = gcnew Drawing::Font("Consolas", 8);
            lbl->Location = Point(0, y);
            lbl->AutoSize = true;
            y += 18;
            return lbl;
        }

        NumericUpDown^ addNum(double min, double max, double val, int dec, int& y) {
            NumericUpDown^ nud = gcnew NumericUpDown();
            nud->Location = Point(0, y);
            nud->Width = 200;
            nud->Minimum = (Decimal)min;
            nud->Maximum = (Decimal)max;
            nud->Value = (Decimal)val;
            nud->DecimalPlaces = dec;
            StyleNud(nud);
            y += 32;
            return nud;
        }

        // ============================================================
        //  Вкладка «Параметры»
        // ============================================================
        void BuildParamsTab(TabPage^ tab)
        {
            auto layout = gcnew TableLayoutPanel();
            layout->Dock = DockStyle::Fill;
            layout->ColumnCount = 2;
            layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent, 38));
            layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent, 62));
            layout->Padding = SWF::Padding(12);
            layout->BackColor = Color::Transparent;

            // --- Левая панель: настройки ---
            auto left = gcnew Panel();
            left->Dock = DockStyle::Fill;
            left->BackColor = Color::Transparent;

            int y = 0;

            // Метод
            SWF::Label^ iterMeth = addLabel("Итерационный метод:", y);
            left->Controls->Add(iterMeth);
            rbSOR = gcnew RadioButton(); rbSOR->Text = "Метод верхней релаксации (МВР)";
            rbSeidel = gcnew RadioButton(); rbSeidel->Text = "Метод Зейделя (ω = 1)";
            rbSOR->Checked = true;
            rbSOR->Location = Point(0, y); rbSOR->AutoSize = true;
            StyleRb(rbSOR); left->Controls->Add(rbSOR); y += 22;
            rbSeidel->Location = Point(0, y); rbSeidel->AutoSize = true;
            StyleRb(rbSeidel); left->Controls->Add(rbSeidel); y += 30;

            // n, m
            SWF::Label^ lblXN = addLabel("Число разбиений по X (n):", y);
            left->Controls->Add(lblXN);
            nudN = addNum(4, 500, 40, 0, y);
            left->Controls->Add(nudN);
            SWF::Label^ lblYM = addLabel("Число разбиений по Y (m):", y);
            left->Controls->Add(lblYM);
            nudM = addNum(4, 500, 40, 0, y);
            left->Controls->Add(nudM);

            // omega
            SWF::Label^ lblOmega = addLabel("Параметр релаксации ω:", y);
            left->Controls->Add(lblOmega);
            chkAutoOmega = gcnew CheckBox();
            chkAutoOmega->Text = "Авто (оптимальный)";
            chkAutoOmega->Checked = true;
            chkAutoOmega->Location = Point(0, y);
            chkAutoOmega->AutoSize = true;
            StyleCb(chkAutoOmega); left->Controls->Add(chkAutoOmega); y += 24;
            nudOmega = addNum(0.01, 1.999, 1.9, 4, y);
            left->Controls->Add(nudOmega);
            nudOmega->Enabled = false;
            chkAutoOmega->CheckedChanged += gcnew EventHandler(this, &MyForm::OnAutoOmega);

            // eps
            SWF::Label^ lblEps = addLabel("Точность метода εмет:", y);
            left->Controls->Add(lblEps);
            nudEpsMet = gcnew NumericUpDown();
            nudEpsMet->Location = Point(0, y); nudEpsMet->Width = 200;
            nudEpsMet->Minimum = (Decimal)1e-12; nudEpsMet->Maximum = (Decimal)0.1;
            nudEpsMet->Value = (Decimal)1e-7;
            nudEpsMet->DecimalPlaces = 10;
            nudEpsMet->Increment = (Decimal)1e-8;
            StyleNud(nudEpsMet); left->Controls->Add(nudEpsMet); y += 32;

            SWF::Label^ lblNmax = addLabel("Макс. число итераций Nmax:", y);
            left->Controls->Add(lblNmax);
            nudNmax = gcnew NumericUpDown();
            nudNmax->Location = Point(0, y); nudNmax->Width = 200;
            nudNmax->Minimum = 100; nudNmax->Maximum = 200000;
            nudNmax->Value = 50000;
            nudNmax->DecimalPlaces = 0;
            StyleNud(nudNmax); left->Controls->Add(nudNmax); y += 32;

            y += 10;

            // Кнопки
            btnRunTest = gcnew Button();
            btnRunTest->Text = "▶  Решить тестовую задачу";
            btnRunTest->Location = Point(0, y); btnRunTest->Width = 220; btnRunTest->Height = 36;
            StyleBtn(btnRunTest, Color::FromArgb(40, 80, 160));
            btnRunTest->Click += gcnew EventHandler(this, &MyForm::OnRunTest);
            left->Controls->Add(btnRunTest); y += 46;

            btnRunMain = gcnew Button();
            btnRunMain->Text = "▶  Решить основную задачу";
            btnRunMain->Location = Point(0, y); btnRunMain->Width = 220; btnRunMain->Height = 36;
            StyleBtn(btnRunMain, Color::FromArgb(40, 130, 80));
            btnRunMain->Click += gcnew EventHandler(this, &MyForm::OnRunMain);
            left->Controls->Add(btnRunMain); y += 46;

            btnRunBoth = gcnew Button();
            btnRunBoth->Text = "▶▶  Решить обе задачи";
            btnRunBoth->Location = Point(0, y); btnRunBoth->Width = 220; btnRunBoth->Height = 36;
            StyleBtn(btnRunBoth, Color::FromArgb(100, 50, 140));
            btnRunBoth->Click += gcnew EventHandler(this, &MyForm::OnRunBoth);
            left->Controls->Add(btnRunBoth); y += 46;

            left->AutoScroll = true;

            // --- Правая панель: описание задачи ---
            auto right = gcnew RichTextBox();
            right->Dock = DockStyle::Fill;
            right->ReadOnly = true;
            right->BackColor = Color::FromArgb(14, 16, 26);
            right->ForeColor = Color::FromArgb(180, 200, 255);
            right->Font = gcnew Drawing::Font("Consolas", 9);
            right->BorderStyle = BorderStyle::None;
            right->Text =
                "ВАРИАНТ 1.  Задача Дирихле для уравнения Пуассона\r\n"
                "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\r\n\r\n"
                "Область: x∈[0,1], y∈[0,1]\r\n\r\n"
                "ОСНОВНАЯ ЗАДАЧА:\r\n"
                "  Δu(x,y) = −f(x,y),\r\n"
                "  f(x,y) = sin²(πxy)\r\n\r\n"
                "  Граничные условия:\r\n"
                "  u(0,y) = sin(πy)\r\n"
                "  u(1,y) = sin(πy)\r\n"
                "  u(x,0) = x − x²\r\n"
                "  u(x,1) = x − x²\r\n\r\n"
                "ТЕСТОВАЯ ЗАДАЧА:\r\n"
                "  u*(x,y) = exp(sin²(πxy))\r\n"
                "  Δu* = -(f*),  f* вычисляется аналитически\r\n\r\n"
                "МЕТОД: верхняя релаксация (МВР) / Зейдель\r\n"
                "  Схема 2-го порядка аппроксимации (5-точечный шаблон)\r\n\r\n"
                "ОЦЕНКИ ПОГРЕШНОСТИ:\r\n"
                "  ε₁ = max|u*(xi,yj) − v(N)(xi,yj)|  [тестовая]\r\n"
                "  ε₂ = max|v(N)−v₂(N₂)|              [основная]\r\n"
                "  Требование: ε ≤ 0.5·10⁻⁶\r\n\r\n"
                "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\r\n"
                "Нажмите кнопку слева для запуска расчёта.";

            layout->Controls->Add(left, 0, 0);
            layout->Controls->Add(right, 1, 0);
            tab->Controls->Add(layout);
        }

        // ============================================================
        //  Вкладка «Тестовая задача»
        // ============================================================
        void BuildTestTab(TabPage^ tab)
        {
            auto split = gcnew SplitContainer();
            split->Dock = DockStyle::Fill;
            split->Orientation = Orientation::Horizontal;
            split->SplitterDistance = 280;
            split->BackColor = Color::Transparent;

            // Справка
            rtbTest = gcnew RichTextBox();
            rtbTest->Dock = DockStyle::Fill;
            rtbTest->ReadOnly = true;
            rtbTest->BackColor = Color::FromArgb(14, 16, 26);
            rtbTest->ForeColor = Color::FromArgb(190, 210, 255);
            rtbTest->Font = gcnew Drawing::Font("Consolas", 8.5f);
            rtbTest->BorderStyle = BorderStyle::None;
            rtbTest->Text = "Запустите расчёт тестовой задачи на вкладке «Параметры».";
            split->Panel1->Controls->Add(rtbTest);

            // Графики (4 панели: u*, v(0), v(N), разность)
            pnlTestGraphs = gcnew Panel();
            pnlTestGraphs->Dock = DockStyle::Fill;
            pnlTestGraphs->BackColor = Color::FromArgb(18, 20, 30);

            sfExact    = gcnew SurfacePlot();
            sfInitTest = gcnew SurfacePlot();
            sfNumTest  = gcnew SurfacePlot();
            sfDiffTest = gcnew SurfacePlot();
            for each (SurfacePlot ^ sf in gcnew array<SurfacePlot^>{sfExact, sfInitTest, sfNumTest, sfDiffTest}) {
                sf->BackColor = Color::FromArgb(18, 20, 30);
                pnlTestGraphs->Controls->Add(sf);
            }
            pnlTestGraphs->Resize += gcnew EventHandler(this, &MyForm::OnResizeTestGraphs);

            split->Panel2->Controls->Add(pnlTestGraphs);
            tab->Controls->Add(split);
        }

        // ============================================================
        //  Вкладка «Основная задача»
        // ============================================================
        void BuildMainTab(TabPage^ tab)
        {
            auto split = gcnew SplitContainer();
            split->Dock = DockStyle::Fill;
            split->Orientation = Orientation::Horizontal;
            split->SplitterDistance = 280;
            split->BackColor = Color::Transparent;

            rtbMain = gcnew RichTextBox();
            rtbMain->Dock = DockStyle::Fill;
            rtbMain->ReadOnly = true;
            rtbMain->BackColor = Color::FromArgb(14, 16, 26);
            rtbMain->ForeColor = Color::FromArgb(190, 210, 255);
            rtbMain->Font = gcnew Drawing::Font("Consolas", 8.5f);
            rtbMain->BorderStyle = BorderStyle::None;
            rtbMain->Text = "Запустите расчёт основной задачи на вкладке «Параметры».";
            split->Panel1->Controls->Add(rtbMain);

            pnlMainGraphs = gcnew Panel();
            pnlMainGraphs->Dock = DockStyle::Fill;
            pnlMainGraphs->BackColor = Color::FromArgb(18, 20, 30);

            // 5 панелей: v(0), v(N), разность   /   v2(0), v2(N2)
            sfInitMain  = gcnew SurfacePlot();
            sfMain      = gcnew SurfacePlot();
            sfDiffMain  = gcnew SurfacePlot();
            sfInitMain2 = gcnew SurfacePlot();
            sfMain2     = gcnew SurfacePlot();
            for each (SurfacePlot ^ sf in gcnew array<SurfacePlot^>{
                sfInitMain, sfMain, sfDiffMain, sfInitMain2, sfMain2}) {
                sf->BackColor = Color::FromArgb(18, 20, 30);
                pnlMainGraphs->Controls->Add(sf);
            }
            pnlMainGraphs->Resize += gcnew EventHandler(this, &MyForm::OnResizeMainGraphs);

            split->Panel2->Controls->Add(pnlMainGraphs);
            tab->Controls->Add(split);
        }

        // ============================================================
        //  Обработчики событий
        // ============================================================
        void OnAutoOmega(Object^, EventArgs^) {
            nudOmega->Enabled = !chkAutoOmega->Checked;
        }

        void OnResizeTestGraphs(Object^, EventArgs^) {
            LayoutGrid(pnlTestGraphs,
                gcnew array<SurfacePlot^>{sfExact, sfInitTest, sfNumTest, sfDiffTest},
                2, 2);
        }
        void OnResizeMainGraphs(Object^, EventArgs^) {
            LayoutGrid(pnlMainGraphs,
                gcnew array<SurfacePlot^>{sfInitMain, sfMain, sfDiffMain, sfInitMain2, sfMain2},
                2, 3);
        }

        // Раскладывает массив панелей по сетке rows×cols (порядок — построчно).
        // Лишние ячейки в последней строке остаются пустыми.
        void LayoutGrid(Panel^ parent, array<SurfacePlot^>^ panels, int rows, int cols)
        {
            const int pad = 4;
            int totalW = parent->Width;
            int totalH = parent->Height;
            int w = (totalW - (cols + 1) * pad) / cols;
            int h = (totalH - (rows + 1) * pad) / rows;
            if (w < 1) w = 1;
            if (h < 1) h = 1;
            for (int k = 0; k < panels->Length; ++k) {
                int r = k / cols;
                int c = k % cols;
                panels[k]->Bounds = Rectangle(
                    pad + c * (w + pad),
                    pad + r * (h + pad),
                    w, h);
            }
        }

        void OnRunTest(Object^ sender, EventArgs^ e) { RunTest(); }
        void OnRunMain(Object^ sender, EventArgs^ e) { RunMain(); }
        void OnRunBoth(Object^ sender, EventArgs^) { RunTest(); RunMain(); }

        // ============================================================
        //  Решение тестовой задачи
        // ============================================================
        void RunTest()
        {
            Cursor = Cursors::WaitCursor;
            try {
                int n = (int)nudN->Value;
                int m = (int)nudM->Value;
                int Nmax = (int)nudNmax->Value;
                double epsMet = (double)nudEpsMet->Value;
                double omega;
                if (chkAutoOmega->Checked)
                    omega = PoissonSolver::optimalOmega(n, m);
                else
                    omega = (double)nudOmega->Value;

                PoissonSolver::Method method = rbSOR->Checked ?
                    PoissonSolver::Method::SOR : PoissonSolver::Method::Seidel;

                PoissonSolver solver;
                if (resTest) { delete resTest; resTest = nullptr; }
                resTest = new SolverResult(solver.solve(n, m, omega, epsMet, Nmax, true, method));

                // Заполнить поверхности
                if (diffTestData) { delete diffTestData; diffTestData = nullptr; }
                diffTestData = new std::vector<double>((n + 1) * (m + 1));
                for (int i = 0; i <= n; ++i)
                    for (int j = 0; j <= m; ++j)
                        (*diffTestData)[i * (m + 1) + j] =
                        resTest->u[i * (m + 1) + j] - resTest->v[i * (m + 1) + j];

                double a = resTest->a, b = resTest->b, c = resTest->c, d = resTest->d;
                sfExact   ->SetData(&resTest->u,  n, m, a, b, c, d, "Точное решение u*(x,y)");
                sfInitTest->SetData(&resTest->v0, n, m, a, b, c, d, "Начальное приближение v⁽⁰⁾(x,y)");
                sfNumTest ->SetData(&resTest->v,  n, m, a, b, c, d, "Численное решение v⁽ᴺ⁾(x,y)");
                sfDiffTest->SetData(diffTestData, n, m, a, b, c, d, "Разность u* − v⁽ᴺ⁾");
                LayoutGrid(pnlTestGraphs,
                    gcnew array<SurfacePlot^>{sfExact, sfInitTest, sfNumTest, sfDiffTest},
                    2, 2);

                // Справка
                rtbTest->Text = BuildTestReport(*resTest, omega, method);

                tabControl->SelectedIndex = 1;
            }
            finally {
                Cursor = Cursors::Default;
            }
        }

        // ============================================================
        //  Решение основной задачи
        // ============================================================
        void RunMain()
        {
            Cursor = Cursors::WaitCursor;
            try {
                int n = (int)nudN->Value;
                int m = (int)nudM->Value;
                int Nmax = (int)nudNmax->Value;
                double epsMet = (double)nudEpsMet->Value;
                double omega;
                if (chkAutoOmega->Checked)
                    omega = PoissonSolver::optimalOmega(n, m);
                else
                    omega = (double)nudOmega->Value;

                double omega2 = PoissonSolver::optimalOmega(2 * n, 2 * m);

                PoissonSolver::Method method = rbSOR->Checked ?
                    PoissonSolver::Method::SOR : PoissonSolver::Method::Seidel;

                PoissonSolver solver;
                if (resMain) { delete resMain;  resMain = nullptr; }
                if (resMain2) { delete resMain2; resMain2 = nullptr; }
                resMain = new SolverResult(solver.solve(n, m, omega, epsMet * 0.1, Nmax, false, method));
                resMain2 = new SolverResult(solver.solve(2 * n, 2 * m, omega2, epsMet * 0.01, Nmax * 2, false, method));

                int iMax, jMax;
                resMain->epsilon2 = PoissonSolver::computeEpsilon2(*resMain, *resMain2, iMax, jMax);
                resMain->iMaxErr = iMax;
                resMain->jMaxErr = jMax;

                // Разность в общих узлах
                if (diffMainData) { delete diffMainData; diffMainData = nullptr; }
                diffMainData = new std::vector<double>((n + 1) * (m + 1));
                for (int i = 0; i <= n; ++i)
                    for (int j = 0; j <= m; ++j)
                        (*diffMainData)[i * (m + 1) + j] =
                        resMain->v[i * (m + 1) + j] - resMain2->v[(2 * i) * (2 * m + 1) + (2 * j)];

                double a = resMain->a, b = resMain->b, c = resMain->c, d = resMain->d;
                sfInitMain ->SetData(&resMain->v0,  n,     m,     a, b, c, d, "Начальное приближение v⁽⁰⁾(x,y), сетка (n,m)");
                sfMain     ->SetData(&resMain->v,   n,     m,     a, b, c, d, "Численное решение v⁽ᴺ⁾(x,y), сетка (n,m)");
                sfDiffMain ->SetData(diffMainData,  n,     m,     a, b, c, d, "Разность v⁽ᴺ⁾ − v₂⁽ᴺ²⁾");
                sfInitMain2->SetData(&resMain2->v0, 2 * n, 2 * m, a, b, c, d, "Начальное приближение v₂⁽⁰⁾(x,y), сетка (2n,2m)");
                sfMain2    ->SetData(&resMain2->v,  2 * n, 2 * m, a, b, c, d, "Численное решение v₂⁽ᴺ²⁾(x,y), сетка (2n,2m)");
                LayoutGrid(pnlMainGraphs,
                    gcnew array<SurfacePlot^>{sfInitMain, sfMain, sfDiffMain, sfInitMain2, sfMain2},
                    2, 3);

                rtbMain->Text = BuildMainReport(*resMain, *resMain2, omega, omega2, method);

                tabControl->SelectedIndex = 2;
            }
            finally {
                Cursor = Cursors::Default;
            }
        }

        // ============================================================
        //  Формирование справок
        // ============================================================
        String^ BuildTestReport(const SolverResult& r, double omega,
            PoissonSolver::Method method)
        {
            std::ostringstream oss;
            oss << std::fixed;
            oss << "СПРАВКА — ТЕСТОВАЯ ЗАДАЧА\r\n";
            oss << "═══════════════════════════════════════════════════════════════\r\n";
            oss << "Метод: " << (method == PoissonSolver::Method::SOR ?
                "Верхняя релаксация (МВР)" : "Метод Зейделя") << "\r\n";
            oss << "Сетка: n=" << r.n << ", m=" << r.m
                << "  (h=" << std::setprecision(6) << r.h
                << ", k=" << r.k << ")\r\n";
            oss << "Параметр ω = " << std::setprecision(6) << omega << "\r\n";
            oss << "Критерий остановки: εмет = " << std::setprecision(2) << std::scientific
                << (double)nudEpsMet->Value << ", Nmax = " << (int)nudNmax->Value << "\r\n";
            oss << std::fixed;
            oss << "Итераций выполнено: N = " << r.iterDone << "\r\n";
            oss << "Достигнутая точность метода: ε(N) = "
                << std::setprecision(4) << std::scientific << r.methodError << "\r\n";
            oss << "Норма невязки (max): ||R(N)|| = "
                << std::setprecision(4) << std::scientific << r.residualNorm << "\r\n\r\n";
            oss << "Тестовая задача должна быть решена с погрешностью ≤ 0.5·10⁻⁶\r\n";
            oss << "Достигнутая погрешность: ε₁ = "
                << std::setprecision(4) << std::scientific << r.epsilon1 << "\r\n";
            if (r.epsilon1 <= 5e-7)
                oss << "✓ ТРЕБОВАНИЕ ВЫПОЛНЕНО (ε₁ ≤ 0.5·10⁻⁶)\r\n";
            else
                oss << "✗ Требование не выполнено. Увеличьте n, m или уменьшите εмет.\r\n";
            oss << "Максимальное отклонение в узле: x=" << std::fixed << std::setprecision(4)
                << (r.a + r.iMaxErr * r.h) << ", y="
                << (r.c + r.jMaxErr * r.k) << "\r\n";
            oss << "Начальное приближение: линейная интерполяция по x\r\n\r\n";
            oss << "═══════════════════════════════════════════════════════════════\r\n";
            oss << "ОЦЕНКИ ПОГРЕШНОСТИ:\r\n";
            double h2k2 = r.h * r.h + r.k * r.k;
            oss << "  Погрешность итерационного метода:\r\n";
            oss << "    ||Z(N)||∞ ≤ ||Z(N)||₂ ≤ ε(N)/λmin ≈ " << std::setprecision(4)
                << std::scientific << r.methodError << "\r\n";
            oss << "  Погрешность схемы (теорема о сходимости):\r\n";
            oss << "    ||z||∞ ≤ C(h² + k²) ≈ C·" << std::setprecision(6) << std::fixed
                << h2k2 << "  (C — константа)\r\n";
            oss << "  Порядок аппроксимации схемы: 2\r\n\r\n";
            oss << "ТАБЛИЦА ЗНАЧЕНИЙ (первые 6×6 узлов):\r\n";
            oss << "  u*(xi,yj):\r\n";
            int nShow = std::min(6, r.n);
            int mShow = std::min(6, r.m);
            for (int j = 0; j <= mShow; ++j) {
                for (int i = 0; i <= nShow; ++i)
                    oss << std::setw(12) << std::setprecision(6) << r.u[i * (r.m + 1) + j];
                oss << "\r\n";
            }
            oss << "  v(N)(xi,yj):\r\n";
            for (int j = 0; j <= mShow; ++j) {
                for (int i = 0; i <= nShow; ++i)
                    oss << std::setw(12) << std::setprecision(6) << r.v[i * (r.m + 1) + j];
                oss << "\r\n";
            }
            oss << "  Разность u* − v(N):\r\n";
            for (int j = 0; j <= mShow; ++j) {
                for (int i = 0; i <= nShow; ++i)
                    oss << std::setw(12) << std::setprecision(2) << std::scientific
                    << (r.u[i * (r.m + 1) + j] - r.v[i * (r.m + 1) + j]);
                oss << "\r\n";
            }
            return gcnew String(oss.str().c_str());
        }

        String^ BuildMainReport(const SolverResult& r1, const SolverResult& r2,
            double omega, double omega2, PoissonSolver::Method method)
        {
            std::ostringstream oss;
            oss << std::fixed;
            oss << "СПРАВКА — ОСНОВНАЯ ЗАДАЧА\r\n";
            oss << "═══════════════════════════════════════════════════════════════\r\n";
            oss << "Метод: " << (method == PoissonSolver::Method::SOR ?
                "Верхняя релаксация (МВР)" : "Метод Зейделя") << "\r\n\r\n";
            oss << "ОСНОВНАЯ СЕТКА (n=" << r1.n << ", m=" << r1.m << "):\r\n";
            oss << "  ω = " << std::setprecision(6) << omega
                << "  εмет = " << std::setprecision(2) << std::scientific
                << (double)nudEpsMet->Value * 0.1 << "\r\n";
            oss << std::fixed;
            oss << "  Итераций: N = " << r1.iterDone
                << "  ε(N) = " << std::scientific << std::setprecision(4) << r1.methodError << "\r\n";
            oss << "  ||R(N)|| (max) = " << r1.residualNorm << "\r\n\r\n";
            oss << "УТОЧНЁННАЯ СЕТКА (2n=" << r2.n << ", 2m=" << r2.m << "):\r\n";
            oss << "  ω₂ = " << std::fixed << std::setprecision(6) << omega2
                << "  εмет₂ = " << std::setprecision(2) << std::scientific
                << (double)nudEpsMet->Value * 0.01 << "\r\n";
            oss << std::fixed;
            oss << "  Итераций: N₂ = " << r2.iterDone
                << "  ε(N₂) = " << std::scientific << std::setprecision(4) << r2.methodError << "\r\n";
            oss << "  ||R(N₂)|| (max) = " << r2.residualNorm << "\r\n\r\n";
            oss << "Основная задача должна быть решена с точностью ≤ 0.5·10⁻⁶\r\n";
            oss << "Достигнутая точность: ε₂ = "
                << std::scientific << std::setprecision(4) << r1.epsilon2 << "\r\n";
            if (r1.epsilon2 <= 5e-7)
                oss << "✓ ТРЕБОВАНИЕ ВЫПОЛНЕНО (ε₂ ≤ 0.5·10⁻⁶)\r\n";
            else
                oss << "✗ Требование не выполнено. Увеличьте n, m или уменьшите εмет.\r\n";
            oss << "Максимальное отклонение в узле: x=" << std::fixed << std::setprecision(4)
                << (r1.a + r1.iMaxErr * r1.h) << ", y="
                << (r1.c + r1.jMaxErr * r1.k) << "\r\n";
            oss << "Начальное приближение (обе сетки): линейная интерполяция по x\r\n\r\n";
            oss << "═══════════════════════════════════════════════════════════════\r\n";
            oss << "ТАБЛИЦА v(N) (первые 6×6 узлов):\r\n";
            int nShow = std::min(6, r1.n);
            int mShow = std::min(6, r1.m);
            for (int j = 0; j <= mShow; ++j) {
                for (int i = 0; i <= nShow; ++i)
                    oss << std::setw(12) << std::setprecision(6) << std::fixed
                    << r1.v[i * (r1.m + 1) + j];
                oss << "\r\n";
            }
            oss << "ТАБЛИЦА v₂(N₂) в общих узлах (первые 6×6):\r\n";
            for (int j = 0; j <= mShow; ++j) {
                for (int i = 0; i <= nShow; ++i)
                    oss << std::setw(12) << std::setprecision(6) << std::fixed
                    << r2.v[(2 * i) * (2 * r1.m + 1) + (2 * j)];
                oss << "\r\n";
            }
            return gcnew String(oss.str().c_str());
        }

        // ============================================================
        //  Стилизация UI
        // ============================================================
        void StyleTab(TabControl^ tc) {
            tc->BackColor = Color::FromArgb(18, 20, 30);
            tc->Appearance = TabAppearance::FlatButtons;
        }
        void StyleTabPage(TabPage^ tp) {
            tp->BackColor = Color::FromArgb(18, 20, 30);
            tp->ForeColor = Color::FromArgb(200, 210, 255);
        }
        void StyleNud(NumericUpDown^ nud) {
            nud->BackColor = Color::FromArgb(28, 32, 48);
            nud->ForeColor = Color::FromArgb(200, 220, 255);
            nud->BorderStyle = BorderStyle::FixedSingle;
        }
        void StyleBtn(Button^ btn, Color bg) {
            btn->BackColor = bg;
            btn->ForeColor = Color::White;
            btn->FlatStyle = FlatStyle::Flat;
            btn->FlatAppearance->BorderSize = 0;
            btn->Font = gcnew Drawing::Font("Consolas", 9, FontStyle::Bold);
            btn->Cursor = Cursors::Hand;
        }
        void StyleRb(RadioButton^ rb) {
            rb->ForeColor = Color::FromArgb(190, 210, 255);
            rb->BackColor = Color::Transparent;
        }
        void StyleCb(CheckBox^ cb) {
            cb->ForeColor = Color::FromArgb(190, 210, 255);
            cb->BackColor = Color::Transparent;
        }

    };
}
