#pragma once
#include "Solver.h"

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
        DataGridView^ dgvTestExact, ^ dgvTestNum, ^ dgvTestDiff;
        Panel^ pnlTestGraphs;
        SurfacePlot^ sfExact, ^ sfInitTest, ^ sfNumTest, ^ sfDiffTest;

        // Вкладка «Основная задача»
        RichTextBox^ rtbMain;
        DataGridView^ dgvMainNum, ^ dgvMainNum2, ^ dgvMainDiff;
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
            this->WindowState = FormWindowState::Maximized;
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
            lbl->Font = gcnew Drawing::Font("Consolas", 16);
            lbl->Location = Point(0, y);
            lbl->AutoSize = true;
            y += 32;
            return lbl;
        }

        NumericUpDown^ addNum(double min, double max, double val, int dec, int& y) {
            NumericUpDown^ nud = gcnew NumericUpDown();
            nud->Location = Point(0, y);
            nud->Width = 260;
            nud->Font = gcnew Drawing::Font("Consolas", 14);
            nud->Minimum = (Decimal)min;
            nud->Maximum = (Decimal)max;
            nud->Value = (Decimal)val;
            nud->DecimalPlaces = dec;
            StyleNud(nud);
            y += 42;
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
            rbSOR->Font = gcnew Drawing::Font("Consolas", 14);
            rbSeidel = gcnew RadioButton(); rbSeidel->Text = "Метод Зейделя";
            rbSeidel->Font = gcnew Drawing::Font("Consolas", 14);
            rbSOR->Checked = true;
            rbSOR->Location = Point(0, y); rbSOR->AutoSize = true;
            StyleRb(rbSOR); left->Controls->Add(rbSOR); y += 30;
            rbSeidel->Location = Point(0, y); rbSeidel->AutoSize = true;
            StyleRb(rbSeidel); left->Controls->Add(rbSeidel); y += 40;

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
            chkAutoOmega->Font = gcnew Drawing::Font("Consolas", 14);
            chkAutoOmega->Checked = true;
            chkAutoOmega->Location = Point(0, y);
            chkAutoOmega->AutoSize = true;
            StyleCb(chkAutoOmega); left->Controls->Add(chkAutoOmega); y += 34;
            nudOmega = addNum(0.01, 1.999, 1.9, 4, y);
            left->Controls->Add(nudOmega);
            nudOmega->Enabled = false;
            chkAutoOmega->CheckedChanged += gcnew EventHandler(this, &MyForm::OnAutoOmega);

            // eps
            SWF::Label^ lblEps = addLabel("Точность метода εмет:", y);
            left->Controls->Add(lblEps);
            nudEpsMet = gcnew NumericUpDown();
            nudEpsMet->Location = Point(0, y); nudEpsMet->Width = 260;
            nudEpsMet->Font = gcnew Drawing::Font("Consolas", 14);
            nudEpsMet->Minimum = (Decimal)1e-12; nudEpsMet->Maximum = (Decimal)0.1;
            nudEpsMet->Value = (Decimal)1e-7;
            nudEpsMet->DecimalPlaces = 10;
            nudEpsMet->Increment = (Decimal)1e-8;
            StyleNud(nudEpsMet); left->Controls->Add(nudEpsMet); y += 42;

            SWF::Label^ lblNmax = addLabel("Макс. число итераций Nmax:", y);
            left->Controls->Add(lblNmax);
            nudNmax = gcnew NumericUpDown();
            nudNmax->Location = Point(0, y); nudNmax->Width = 260;
            nudNmax->Font = gcnew Drawing::Font("Consolas", 14);
            nudNmax->Minimum = 100; nudNmax->Maximum = 200000;
            nudNmax->Value = 50000;
            nudNmax->DecimalPlaces = 0;
            StyleNud(nudNmax); left->Controls->Add(nudNmax); y += 42;

            y += 16;

            // Кнопки
            btnRunTest = gcnew Button();
            btnRunTest->Text = "Решить тестовую задачу";
            btnRunTest->Location = Point(0, y); btnRunTest->Width = 280; btnRunTest->Height = 44;
            StyleBtn(btnRunTest, Color::FromArgb(40, 80, 160));
            btnRunTest->Click += gcnew EventHandler(this, &MyForm::OnRunTest);
            left->Controls->Add(btnRunTest); y += 56;

            btnRunMain = gcnew Button();
            btnRunMain->Text = "Решить основную задачу";
            btnRunMain->Location = Point(0, y); btnRunMain->Width = 280; btnRunMain->Height = 44;
            StyleBtn(btnRunMain, Color::FromArgb(40, 130, 80));
            btnRunMain->Click += gcnew EventHandler(this, &MyForm::OnRunMain);
            left->Controls->Add(btnRunMain); y += 56;

            btnRunBoth = gcnew Button();
            btnRunBoth->Text = "Решить обе задачи";
            btnRunBoth->Location = Point(0, y); btnRunBoth->Width = 280; btnRunBoth->Height = 44;
            StyleBtn(btnRunBoth, Color::FromArgb(100, 50, 140));
            btnRunBoth->Click += gcnew EventHandler(this, &MyForm::OnRunBoth);
            left->Controls->Add(btnRunBoth); y += 56;

            left->AutoScroll = true;

            // --- Правая панель: описание задачи ---
            auto right = gcnew RichTextBox();
            right->Dock = DockStyle::Fill;
            right->ReadOnly = true;
            right->BackColor = Color::FromArgb(14, 16, 26);
            right->ForeColor = Color::FromArgb(180, 200, 255);
            right->Font = gcnew Drawing::Font("Consolas", 16);
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
            split->Orientation = Orientation::Vertical;
            //split->SplitterDistance = 320;
            split->BackColor = Color::Transparent;

            // Верхняя секция: вкладки «Справка / u* / v(N) / разность»
            auto tabsInfo = gcnew TabControl();
            tabsInfo->Dock = DockStyle::Fill;
            StyleTab(tabsInfo);

            auto tpReport = gcnew TabPage("  Справка  "); StyleTabPage(tpReport);
            rtbTest = gcnew RichTextBox();
            rtbTest->Dock = DockStyle::Fill;
            rtbTest->ReadOnly = true;
            rtbTest->BackColor = Color::FromArgb(14, 16, 26);
            rtbTest->ForeColor = Color::FromArgb(190, 210, 255);
            rtbTest->Font = gcnew Drawing::Font("Consolas", 16);
            rtbTest->BorderStyle = BorderStyle::None;
            rtbTest->Text = "Запустите расчёт тестовой задачи на вкладке «Параметры».";
            tpReport->Controls->Add(rtbTest);

            auto tpExact = gcnew TabPage("  u*(x,y)  "); StyleTabPage(tpExact);
            dgvTestExact = MakeGridView();
            tpExact->Controls->Add(dgvTestExact);

            auto tpNum = gcnew TabPage("  v⁽ᴺ⁾(x,y)  "); StyleTabPage(tpNum);
            dgvTestNum = MakeGridView();
            tpNum->Controls->Add(dgvTestNum);

            auto tpDiff = gcnew TabPage("  u* − v⁽ᴺ⁾  "); StyleTabPage(tpDiff);
            dgvTestDiff = MakeGridView();
            tpDiff->Controls->Add(dgvTestDiff);

            tabsInfo->TabPages->AddRange(gcnew array<TabPage^>{tpReport, tpExact, tpNum, tpDiff});
            split->Panel1->Controls->Add(tabsInfo);

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
            split->Orientation = Orientation::Vertical;
            //split->SplitterDistance = 320;
            split->BackColor = Color::Transparent;

            // Верхняя секция: вкладки «Справка / v(N) / v2(N2) / разность»
            auto tabsInfo = gcnew TabControl();
            tabsInfo->Dock = DockStyle::Fill;
            StyleTab(tabsInfo);

            auto tpReport = gcnew TabPage("  Справка  "); StyleTabPage(tpReport);
            rtbMain = gcnew RichTextBox();
            rtbMain->Dock = DockStyle::Fill;
            rtbMain->ReadOnly = true;
            rtbMain->BackColor = Color::FromArgb(14, 16, 26);
            rtbMain->ForeColor = Color::FromArgb(190, 210, 255);
            rtbMain->Font = gcnew Drawing::Font("Consolas", 16);
            rtbMain->BorderStyle = BorderStyle::None;
            rtbMain->Text = "Запустите расчёт основной задачи на вкладке «Параметры».";
            tpReport->Controls->Add(rtbMain);

            auto tpNum = gcnew TabPage("  v⁽ᴺ⁾(x,y)  "); StyleTabPage(tpNum);
            dgvMainNum = MakeGridView();
            tpNum->Controls->Add(dgvMainNum);

            auto tpNum2 = gcnew TabPage("  v₂⁽ᴺ²⁾(x,y)  "); StyleTabPage(tpNum2);
            dgvMainNum2 = MakeGridView();
            tpNum2->Controls->Add(dgvMainNum2);

            auto tpDiff = gcnew TabPage("  v⁽ᴺ⁾ − v₂⁽ᴺ²⁾  "); StyleTabPage(tpDiff);
            dgvMainDiff = MakeGridView();
            tpDiff->Controls->Add(dgvMainDiff);

            tabsInfo->TabPages->AddRange(gcnew array<TabPage^>{tpReport, tpNum, tpNum2, tpDiff});
            split->Panel1->Controls->Add(tabsInfo);

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

                // Таблицы
                FillGrid(dgvTestExact, &resTest->u,  n, m, a, b, c, d, false);
                FillGrid(dgvTestNum,   &resTest->v,  n, m, a, b, c, d, false);
                FillGrid(dgvTestDiff,  diffTestData, n, m, a, b, c, d, true);

                // Справка
                BuildTestReport(rtbTest, *resTest, omega, method);

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

                // Таблицы — v2 проектируется в общие узлы основной сетки (i,j) ↔ (2i,2j)
                std::vector<double> v2Coarse((n + 1) * (m + 1));
                for (int i = 0; i <= n; ++i)
                    for (int j = 0; j <= m; ++j)
                        v2Coarse[i * (m + 1) + j] = resMain2->v[(2 * i) * (2 * m + 1) + (2 * j)];

                FillGrid(dgvMainNum,  &resMain->v,  n, m, a, b, c, d, false);
                FillGrid(dgvMainNum2, &v2Coarse,    n, m, a, b, c, d, false);
                FillGrid(dgvMainDiff, diffMainData, n, m, a, b, c, d, true);

                BuildMainReport(rtbMain, *resMain, *resMain2, omega, omega2, method);

                tabControl->SelectedIndex = 2;
            }
            finally {
                Cursor = Cursors::Default;
            }
        }

        // ============================================================
        //  Формирование справок
        // ============================================================
        void BuildTestReport(RichTextBox^ rtb, const SolverResult& r, double omega,
            PoissonSolver::Method method)
        {
            rtb->Clear();
            String^ methodName = (method == PoissonSolver::Method::SOR) ?
                "Верхняя релаксация (МВР)" : "Метод Зейделя";

            rtb->AppendText("СПРАВКА — ТЕСТОВАЯ ЗАДАЧА\r\n");
            rtb->AppendText("═══════════════════════════════════════════════════════════════\r\n");
            rtb->AppendText(String::Format("Метод: {0}\r\n", methodName));
            rtb->AppendText(String::Format("Сетка: n={0}, m={1}  (h={2:F6}, k={3:F6})\r\n",
                r.n, r.m, r.h, r.k));
            rtb->AppendText(String::Format("Параметр ω = {0:F6}\r\n", omega));
            rtb->AppendText(String::Format("Критерий остановки: εмет = {0:E2}, Nmax = {1}\r\n",
                (double)nudEpsMet->Value, (int)nudNmax->Value));
            rtb->AppendText(String::Format("Итераций выполнено: N = {0}\r\n", r.iterDone));
            rtb->AppendText(String::Format("Достигнутая точность метода: ε(N) = {0:E4}\r\n",
                r.methodError));
            rtb->AppendText(String::Format("Норма невязки (max): ||R(N)|| = {0:E4}\r\n\r\n",
                r.residualNorm));

            rtb->AppendText("Тестовая задача должна быть решена с погрешностью ≤ 0.5·10⁻⁶\r\n");
            rtb->AppendText(String::Format("Достигнутая погрешность: ε₁ = {0:E4}\r\n", r.epsilon1));
            if (r.epsilon1 <= 5e-7)
                rtb->AppendText("✓ ТРЕБОВАНИЕ ВЫПОЛНЕНО (ε₁ ≤ 0.5·10⁻⁶)\r\n");
            else
                rtb->AppendText("✗ Требование не выполнено. Увеличьте n, m или уменьшите εмет.\r\n");
            rtb->AppendText(String::Format("Максимальное отклонение в узле: x={0:F4}, y={1:F4}\r\n",
                r.a + r.iMaxErr * r.h, r.c + r.jMaxErr * r.k));
            rtb->AppendText("Начальное приближение: линейная интерполяция по x\r\n\r\n");

            rtb->AppendText("═══════════════════════════════════════════════════════════════\r\n");
            rtb->AppendText("ОЦЕНКИ ПОГРЕШНОСТИ:\r\n");
            double h2k2 = r.h * r.h + r.k * r.k;
            rtb->AppendText("  Погрешность итерационного метода:\r\n");
            rtb->AppendText(String::Format("    ||Z(N)||∞ ≤ ||Z(N)||₂ ≤ ε(N)/λmin ≈ {0:E4}\r\n",
                r.methodError));
            rtb->AppendText("  Погрешность схемы (теорема о сходимости):\r\n");
            rtb->AppendText(String::Format("    ||z||∞ ≤ C(h² + k²) ≈ C·{0:F6}  (C — константа)\r\n",
                h2k2));
            rtb->AppendText("  Порядок аппроксимации схемы: 2\r\n\r\n");

            rtb->AppendText("Таблицы значений (u*, v⁽ᴺ⁾, разность) — см. вкладки выше.\r\n");
        }

        void BuildMainReport(RichTextBox^ rtb, const SolverResult& r1, const SolverResult& r2,
            double omega, double omega2, PoissonSolver::Method method)
        {
            rtb->Clear();
            String^ methodName = (method == PoissonSolver::Method::SOR) ?
                "Верхняя релаксация (МВР)" : "Метод Зейделя";

            rtb->AppendText("СПРАВКА — ОСНОВНАЯ ЗАДАЧА\r\n");
            rtb->AppendText("═══════════════════════════════════════════════════════════════\r\n");
            rtb->AppendText(String::Format("Метод: {0}\r\n\r\n", methodName));

            rtb->AppendText(String::Format("ОСНОВНАЯ СЕТКА (n={0}, m={1}):\r\n", r1.n, r1.m));
            rtb->AppendText(String::Format("  ω = {0:F6}  εмет = {1:E2}\r\n",
                omega, (double)nudEpsMet->Value * 0.1));
            rtb->AppendText(String::Format("  Итераций: N = {0}  ε(N) = {1:E4}\r\n",
                r1.iterDone, r1.methodError));
            rtb->AppendText(String::Format("  ||R(N)|| (max) = {0:E4}\r\n\r\n", r1.residualNorm));

            rtb->AppendText(String::Format("УТОЧНЁННАЯ СЕТКА (2n={0}, 2m={1}):\r\n", r2.n, r2.m));
            rtb->AppendText(String::Format("  ω₂ = {0:F6}  εмет₂ = {1:E2}\r\n",
                omega2, (double)nudEpsMet->Value * 0.01));
            rtb->AppendText(String::Format("  Итераций: N₂ = {0}  ε(N₂) = {1:E4}\r\n",
                r2.iterDone, r2.methodError));
            rtb->AppendText(String::Format("  ||R(N₂)|| (max) = {0:E4}\r\n\r\n", r2.residualNorm));

            rtb->AppendText("Основная задача должна быть решена с точностью ≤ 0.5·10⁻⁶\r\n");
            rtb->AppendText(String::Format("Достигнутая точность: ε₂ = {0:E4}\r\n", r1.epsilon2));
            if (r1.epsilon2 <= 5e-7)
                rtb->AppendText("✓ ТРЕБОВАНИЕ ВЫПОЛНЕНО (ε₂ ≤ 0.5·10⁻⁶)\r\n");
            else
                rtb->AppendText("✗ Требование не выполнено. Увеличьте n, m или уменьшите εмет.\r\n");
            rtb->AppendText(String::Format("Максимальное отклонение в узле: x={0:F4}, y={1:F4}\r\n",
                r1.a + r1.iMaxErr * r1.h, r1.c + r1.jMaxErr * r1.k));
            rtb->AppendText("Начальное приближение (обе сетки): линейная интерполяция по x\r\n\r\n");

            rtb->AppendText("═══════════════════════════════════════════════════════════════\r\n");
            rtb->AppendText("Таблицы значений (v⁽ᴺ⁾, v₂⁽ᴺ²⁾, разность) — см. вкладки выше.\r\n");
        }

        // ============================================================
        //  Таблицы значений сеточной функции
        // ============================================================
        DataGridView^ MakeGridView()
        {
            auto dgv = gcnew DataGridView();
            dgv->Dock = DockStyle::Fill;
            dgv->BackgroundColor = Color::FromArgb(14, 16, 26);
            dgv->BorderStyle = BorderStyle::None;
            dgv->Font = gcnew Drawing::Font("Consolas", 8);
            dgv->ReadOnly = true;
            dgv->AllowUserToAddRows = false;
            dgv->AllowUserToDeleteRows = false;
            dgv->AllowUserToResizeRows = false;
            dgv->RowHeadersVisible = false;
            dgv->ColumnHeadersHeightSizeMode = DataGridViewColumnHeadersHeightSizeMode::DisableResizing;
            dgv->ColumnHeadersHeight = 28;
            dgv->SelectionMode = DataGridViewSelectionMode::CellSelect;
            dgv->EnableHeadersVisualStyles = false;
            dgv->GridColor = Color::FromArgb(40, 44, 60);

            dgv->DefaultCellStyle->BackColor = Color::FromArgb(14, 16, 26);
            dgv->DefaultCellStyle->ForeColor = Color::FromArgb(190, 210, 255);
            dgv->DefaultCellStyle->SelectionBackColor = Color::FromArgb(60, 80, 140);
            dgv->DefaultCellStyle->SelectionForeColor = Color::White;
            dgv->DefaultCellStyle->Alignment = DataGridViewContentAlignment::MiddleRight;

            dgv->ColumnHeadersDefaultCellStyle->BackColor = Color::FromArgb(28, 32, 48);
            dgv->ColumnHeadersDefaultCellStyle->ForeColor = Color::FromArgb(200, 220, 255);
            dgv->ColumnHeadersDefaultCellStyle->Alignment = DataGridViewContentAlignment::MiddleCenter;
            dgv->ColumnHeadersDefaultCellStyle->Font = gcnew Drawing::Font("Consolas", 8, FontStyle::Bold);
            return dgv;
        }

        // Заполнить DataGridView сеточной функцией data[i*(m+1)+j] на [a,b]×[c,d].
        // Строки — по y (сверху вниз: y_m → y_0), столбцы — по x (i = 0..n).
        void FillGrid(DataGridView^ dgv, std::vector<double>* data, int n, int m,
                      double a, double b, double c, double d, bool scientificFmt)
        {
            dgv->SuspendLayout();
            dgv->Rows->Clear();
            dgv->Columns->Clear();

            if (!data || data->empty() || n <= 0 || m <= 0) {
                dgv->ResumeLayout();
                return;
            }

            int Ny = m + 1;

            // Первый столбец — значение y (фиксированный)
            auto colY = gcnew DataGridViewTextBoxColumn();
            colY->HeaderText = "y \\ x";
            colY->Width = 80;
            colY->Frozen = true;
            colY->SortMode = DataGridViewColumnSortMode::NotSortable;
            colY->DefaultCellStyle->BackColor = Color::FromArgb(28, 32, 48);
            colY->DefaultCellStyle->ForeColor = Color::FromArgb(200, 220, 255);
            colY->DefaultCellStyle->Font = gcnew Drawing::Font("Consolas", 8, FontStyle::Bold);
            colY->DefaultCellStyle->Alignment = DataGridViewContentAlignment::MiddleCenter;
            dgv->Columns->Add(colY);

            // Столбцы — значения x_i
            for (int i = 0; i <= n; ++i) {
                double xi = a + (b - a) * (double)i / n;
                auto col = gcnew DataGridViewTextBoxColumn();
                col->HeaderText = String::Format("{0:F3}", xi);
                col->Width = 78;
                col->SortMode = DataGridViewColumnSortMode::NotSortable;
                dgv->Columns->Add(col);
            }

            // Строки — для каждого y_j, сверху вниз (j = m, m-1, ..., 0)
            String^ fmt = scientificFmt ? "{0:E3}" : "{0:F6}";
            array<Object^>^ cells = gcnew array<Object^>(n + 2);
            for (int j = m; j >= 0; --j) {
                double yj = c + (d - c) * (double)j / m;
                cells[0] = String::Format("{0:F3}", yj);
                for (int i = 0; i <= n; ++i)
                    cells[i + 1] = String::Format(fmt, (*data)[i * Ny + j]);
                dgv->Rows->Add(cells);
            }

            dgv->ResumeLayout();
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
            btn->Font = gcnew Drawing::Font("Consolas", 14, FontStyle::Bold);
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
