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
    //  Вспомогательный класс для 3D-графика (surface plot в 2D проекции)
    // ================================================================
    ref class SurfacePanel : public Panel
    {
    public:
        std::vector<double>* data;
        int N, M;
        double zMin, zMax;
        String^ title;

        SurfacePanel() : data(nullptr), N(0), M(0), zMin(0), zMax(1) {}

        void SetData(std::vector<double>* d, int n, int m, String^ t) {
            data = d; N = n; M = m; title = t;
            if (data && !data->empty()) {
                zMin = (*data)[0]; zMax = (*data)[0];
                for (auto& v : *data) {
                    if (v < zMin) zMin = v;
                    if (v > zMax) zMax = v;
                }
            }
            Invalidate();
        }

    protected:
        virtual void OnPaint(PaintEventArgs^ e) override {
            Graphics^ g = e->Graphics;
            g->SmoothingMode = SmoothingMode::AntiAlias;
            g->Clear(Color::FromArgb(18, 20, 30));

            if (!data || N == 0 || M == 0) {
                g->DrawString("Нет данных", gcnew Drawing::Font("Consolas", 10), Brushes::Gray, 10, 10);
                return;
            }

            int W = Width, H = Height;
            int margin = 30;
            int pw = W - 2 * margin;
            int ph = H - 2 * margin - 20;

            // Псевдо-3D: изометрическая проекция heatmap + изолинии
            // Рисуем как цветовую карту (heatmap)
            double range = zMax - zMin;
            if (range < 1e-15) range = 1.0;

            int stepX = std::max(1, N / 50);
            int stepY = std::max(1, M / 50);

            for (int i = 0; i < N; i += stepX) {
                for (int j = 0; j < M; j += stepY) {
                    double z = (*data)[i * (M + 1) + j];
                    double t = (z - zMin) / range;

                    // Цвет: синий -> голубой -> зелёный -> жёлтый -> красный
                    int r, gr, bl;
                    if (t < 0.25) {
                        r = 0; gr = (int)(t * 4 * 100);
                        bl = (int)(100 + t * 4 * 155);
                    }
                    else if (t < 0.5) {
                        r = 0; gr = (int)(100 + (t - 0.25) * 4 * 155);
                        bl = (int)(255 - (t - 0.25) * 4 * 255);
                    }
                    else if (t < 0.75) {
                        r = (int)((t - 0.5) * 4 * 255);
                        gr = 255; bl = 0;
                    }
                    else {
                        r = 255; gr = (int)(255 - (t - 0.75) * 4 * 255); bl = 0;
                    }
                    r = std::min(255, std::max(0, r));
                    gr = std::min(255, std::max(0, gr));
                    bl = std::min(255, std::max(0, bl));

                    int px = margin + (int)((double)i / N * pw);
                    int py = margin + ph - (int)((double)j / M * ph);
                    int cw = std::max(2, (int)((double)stepX / N * pw) + 1);
                    int ch = std::max(2, (int)((double)stepY / M * ph) + 1);

                    g->FillRectangle(gcnew SolidBrush(Color::FromArgb(r, gr, bl)),
                        px, py - ch, cw, ch);
                }
            }

            // Рамка
            g->DrawRectangle(gcnew Pen(Color::FromArgb(80, 80, 110), 1),
                margin, margin, pw, ph);

            // Метки осей
            auto lblFont = gcnew Drawing::Font("Consolas", 7);
            auto lblBrush = Brushes::LightGray;
            g->DrawString("x", gcnew Drawing::Font("Consolas", 8, FontStyle::Bold), Brushes::White,
                margin + pw / 2, H - 16);
            g->DrawString("y", gcnew Drawing::Font("Consolas", 8, FontStyle::Bold), Brushes::White,
                2, margin + ph / 2);

            // min/max
            g->DrawString(String::Format("min={0:F4}", zMin), lblFont, lblBrush, margin, H - 16);
            g->DrawString(String::Format("max={0:F4}", zMax), lblFont, lblBrush, W - 90, H - 16);

            // Заголовок
            g->DrawString(title, gcnew Drawing::Font("Consolas", 9, FontStyle::Bold),
                Brushes::White, margin, 4);
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
        SurfacePanel^ sfExact, ^ sfNumTest, ^ sfDiffTest;

        // Вкладка «Основная задача»
        RichTextBox^ rtbMain;
        Panel^ pnlMainGraphs;
        SurfacePanel^ sfMain, ^ sfMain2, ^ sfDiffMain;

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

            // Графики (3 панели)
            pnlTestGraphs = gcnew Panel();
            pnlTestGraphs->Dock = DockStyle::Fill;
            pnlTestGraphs->BackColor = Color::FromArgb(18, 20, 30);

            sfExact = gcnew SurfacePanel(); sfExact->title = "Точное решение u*(x,y)";
            sfNumTest = gcnew SurfacePanel(); sfNumTest->title = "Численное решение v(N)(x,y)";
            sfDiffTest = gcnew SurfacePanel(); sfDiffTest->title = "Разность u* − v(N)";
            for each (SurfacePanel ^ sf in gcnew array<SurfacePanel^>{sfExact, sfNumTest, sfDiffTest}) {
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

            sfMain = gcnew SurfacePanel(); sfMain->title = "v(N)(x,y) на сетке (n,m)";
            sfMain2 = gcnew SurfacePanel(); sfMain2->title = "v₂(N₂)(x,y) на сетке (2n,2m)";
            sfDiffMain = gcnew SurfacePanel(); sfDiffMain->title = "Разность v(N) − v₂(N₂)";
            for each (SurfacePanel ^ sf in gcnew array<SurfacePanel^>{sfMain, sfMain2, sfDiffMain}) {
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

        void OnResizeTestGraphs(Object^, EventArgs^) { LayoutThreePanels(pnlTestGraphs, sfExact, sfNumTest, sfDiffTest); }
        void OnResizeMainGraphs(Object^, EventArgs^) { LayoutThreePanels(pnlMainGraphs, sfMain, sfMain2, sfDiffMain); }

        void LayoutThreePanels(Panel^ parent, SurfacePanel^ p1, SurfacePanel^ p2, SurfacePanel^ p3)
        {
            int w = parent->Width / 3 - 4;
            int h = parent->Height - 4;
            p1->Bounds = Rectangle(0, 2, w, h);
            p2->Bounds = Rectangle(w + 4, 2, w, h);
            p3->Bounds = Rectangle(2 * w + 8, 2, w, h);
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

                sfExact->SetData(&resTest->u, n, m, "Точное решение u*(x,y)");
                sfNumTest->SetData(&resTest->v, n, m, "Численное решение v(N)(x,y)");
                sfDiffTest->SetData(diffTestData, n, m, "Разность u* − v(N)");
                LayoutThreePanels(pnlTestGraphs, sfExact, sfNumTest, sfDiffTest);

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

                sfMain->SetData(&resMain->v, n, m, "v(N)(x,y) на сетке (n,m)");
                sfMain2->SetData(&resMain2->v, 2 * n, 2 * m, "v₂(N₂)(x,y) на сетке (2n,2m)");
                sfDiffMain->SetData(diffMainData, n, m, "Разность v(N) − v₂(N₂)");
                LayoutThreePanels(pnlMainGraphs, sfMain, sfMain2, sfDiffMain);

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
