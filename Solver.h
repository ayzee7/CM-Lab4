#pragma once
#define _USE_MATH_DEFINES

#include <vector>
#include <cmath>
#include <string>
#include <algorithm>

// ============================================================
//  SolverResult – результат одного запуска МВР / Зейделя
// ============================================================
struct SolverResult
{
    int    n, m;
    double h, k;
    double omega;
    int    iterDone;
    double residualNormV0, residualNormVN;
    double methodError;
    double a, b, c, d;

    // Сеточные функции (размер (n+1)*(m+1), индекс i*(m+1)+j)
    std::vector<double> v;   // численное решение
    std::vector<double> v0;  // начальное приближение
    std::vector<double> u;   // точное решение (только для тестовой задачи)
    std::vector<double> F;   // предвычисленная правая часть

    double epsilon1;
    double epsilon2;
    int iMaxErr, jMaxErr;
};

// ============================================================
//  PoissonSolver
// ============================================================
class PoissonSolver
{
public:
    double a = 0, b = 1, c = 0, d = 1;

    enum class Method { SOR, Seidel };

    // ----------------------------------------------------------------
    //  Точное решение тестовой задачи
    // ----------------------------------------------------------------
    static double uExact(double x, double y)
    {
        double s = std::sin(M_PI * x * y);
        return std::exp(s * s);
    }

    // ----------------------------------------------------------------
    //  Правая часть тестовой задачи: -Δu*
    // ----------------------------------------------------------------
    static double fTest(double x, double y)
    {
        double pxy = M_PI * x * y;
        double s = std::sin(pxy);
        double u = std::exp(s * s);
        double sin2 = std::sin(2.0 * pxy);
        double cos2 = std::cos(2.0 * pxy);
        double pi2 = M_PI * M_PI;

        double dxx = u * (pi2 * y * y * sin2 * sin2 + 2.0 * pi2 * y * y * cos2);
        double dyy = u * (pi2 * x * x * sin2 * sin2 + 2.0 * pi2 * x * x * cos2);
        return -(dxx + dyy);
    }

    // ----------------------------------------------------------------
    //  Правая часть основной задачи (вариант 1): sin^2(pi*x*y)
    // ----------------------------------------------------------------
    static double fMain(double x, double y)
    {
        double s = std::sin(M_PI * x * y);
        return s * s;
    }

    // Граничные условия основной задачи (вариант 1)
    static double mu1(double y) { return std::sin(M_PI * y); }
    static double mu2(double y) { return std::sin(M_PI * y); }
    static double mu3(double x) { return x - x * x; }
    static double mu4(double x) { return x - x * x; }

    // ----------------------------------------------------------------
    //  Оптимальный параметр МВР
    // ----------------------------------------------------------------
    static double optimalOmega(int n, int m)
    {
        double rho = 0.5 * (std::cos(M_PI / n) + std::cos(M_PI / m));
        return 2.0 / (1.0 + std::sqrt(1.0 - rho * rho));
    }

    // ================================================================
    //  Основной решатель
    // ================================================================
    SolverResult solve(
        int    n,
        int    m,
        double omega,
        double epsMet,
        int    Nmax,
        bool   isTest,
        Method method = Method::SOR) const
    {
        SolverResult res;
        res.n = n; res.m = m;
        res.a = a; res.b = b; res.c = c; res.d = d;
        res.omega = omega;

        const double h = (b - a) / n;
        const double k = (d - c) / m;
        const double h2 = h * h;
        const double k2 = k * k;

        // FIX 1: предвычислены обратные значения — деление заменяется умножением
        const double inv_h2 = 1.0 / h2;
        const double inv_k2 = 1.0 / k2;
        const double inv_denom = 1.0 / (2.0 * inv_h2 + 2.0 * inv_k2);
        const double om = (method == Method::Seidel) ? 1.0 : omega;
        const double om1 = 1.0 - om;  // для формулы: v = om1*old + om*vNew

        res.h = h; res.k = k;

        const int Nx = n + 1;
        const int Ny = m + 1;
        const int total = Nx * Ny;

        // ============================================================
        //  FIX 2: Предвычисление правой части F[i,j] ДО итераций.
        //
        //  ПРОБЛЕМА в оригинале: F(xi,yj) вызывался через std::function
        //  на каждой итерации внутри двойного цикла.
        //  При n=m=200, Nmax=10000: 200*200*10000 = 400 млн вызовов.
        //  fTest содержит sin, cos, exp — каждый ~10-20 тактов.
        //  Итого: миллиарды тактов только на правую часть.
        //
        //  РЕШЕНИЕ: вычислить F[i,j] один раз, сохранить в массив.
        //  Тогда в итерационном цикле — только чтение из памяти (O(1)).
        // ============================================================
        std::vector<double> Fgrid(total, 0.0);
        for (int i = 1; i < n; ++i) {
            const double xi = a + i * h;
            for (int j = 1; j < m; ++j) {
                const double yj = c + j * k;
                // FIX 3: прямой вызов функции вместо std::function.
                // std::function добавляет косвенный вызов (виртуальная
                // диспетчеризация), который не инлайнится компилятором.
                Fgrid[i * Ny + j] = isTest ? fTest(xi, yj) : fMain(xi, yj);
            }
        }
        res.F = Fgrid;

        auto resNormMax = [&](const std::vector<double>& w) -> double {
            double resNorm = 0.0;
            for (int i = 1; i < n; ++i) {
                const double* vPrev = &w[(i - 1) * Ny];
                const double* vCurr = &w[i * Ny];
                const double* vNext = &w[(i + 1) * Ny];
                const double* Frow = &Fgrid[i * Ny];
                for (int j = 1; j < m; ++j) {
                    double Lv = (vPrev[j] - 2.0 * vCurr[j] + vNext[j]) * inv_h2
                        + (vCurr[j - 1] - 2.0 * vCurr[j] + vCurr[j + 1]) * inv_k2;
                    double aR = std::abs(Frow[j] + Lv);
                    if (aR > resNorm) resNorm = aR;
                }
            }
            return resNorm;
        };

        // ============================================================
        //  Начальное приближение: линейная интерполяция по x
        // ============================================================
        std::vector<double> v(total, 0.0);
        for (int i = 0; i < Nx; ++i) {
            const double xi = a + i * h;
            const double t = (double)i / n;
            for (int j = 0; j < Ny; ++j) {
                const double yj = c + j * k;
                if (i == 0 || i == n || j == 0 || j == m) {
                    if (isTest) {
                        v[i * Ny + j] = uExact(xi, yj);
                    }
                    else {
                        if (i == 0) v[i * Ny + j] = mu1(yj);
                        else if (i == n) v[i * Ny + j] = mu2(yj);
                        else if (j == 0) v[i * Ny + j] = mu3(xi);
                        else             v[i * Ny + j] = mu4(xi);
                    }
                }
                else {
                    double bc0 = isTest ? uExact(a, yj) : mu1(yj);
                    double bcN = isTest ? uExact(b, yj) : mu2(yj);
                    v[i * Ny + j] = (1.0 - t) * bc0 + t * bcN;
                }
            }
        }
        // FIX 4: копируем начальное приближение ПОСЛЕ заполнения (не до)
        res.v0 = v;
        res.residualNormV0 = resNormMax(v);

        // ============================================================
        //  Итерационный процесс МВР / Зейделя
        //
        //  FIX 5: используем указатели на строки массива вместо
        //  вычисления индекса i*Ny+j на каждом шаге.
        //  Компилятор может векторизовать внутренний цикл по j.
        //
        //  FIX 6: внутренний цикл по j — обход памяти последовательный.
        //  v[i*Ny+j..j+1] лежат рядом -> попадают в одну cache-line.
        // ============================================================
        double iterErr = 1e100;
        int    iter = 0;

        while (iter < Nmax && iterErr > epsMet)
        {
            iterErr = 0.0;
            for (int i = 1; i < n; ++i) {
                const double* vPrev = &v[(i - 1) * Ny];
                const double* vCurr = &v[i * Ny];
                const double* vNext = &v[(i + 1) * Ny];
                double* vW = &v[i * Ny];
                const double* Frow = &Fgrid[i * Ny];

                for (int j = 1; j < m; ++j) {
                    const double rhs = Frow[j]
                        + (vPrev[j] + vNext[j]) * inv_h2
                        + (vCurr[j - 1] + vCurr[j + 1]) * inv_k2;

                    const double vNew = rhs * inv_denom;
                    const double old = vW[j];
                    const double updated = om1 * old + om * vNew;

                    const double diff = std::abs(updated - old);
                    if (diff > iterErr) iterErr = diff;
                    vW[j] = updated;
                }
            }
            ++iter;
        }

        res.iterDone = iter;
        res.methodError = iterErr;
        res.v = v;

        // ============================================================
        //  Норма невязки (max)
        //  FIX 7: используем предвычисленный Fgrid — не вызываем F снова
        // ============================================================
        
        res.residualNormVN = resNormMax(v);

        // ============================================================
        //  Погрешность тестовой задачи
        // ============================================================
        res.epsilon1 = 0.0;
        res.iMaxErr = 0;
        res.jMaxErr = 0;
        if (isTest) {
            res.u.resize(total);
            for (int i = 0; i < Nx; ++i) {
                const double xi = a + i * h;
                for (int j = 0; j < Ny; ++j) {
                    const double yj = c + j * k;
                    const double ex = uExact(xi, yj);
                    res.u[i * Ny + j] = ex;
                    const double diff = std::abs(ex - v[i * Ny + j]);
                    if (diff > res.epsilon1) {
                        res.epsilon1 = diff;
                        res.iMaxErr = i;
                        res.jMaxErr = j;
                    }
                }
            }
        }
        res.epsilon2 = 0.0;

        return res;
    }

    // ================================================================
    //  e2 = max|v_coarse - v_fine| в общих узлах
    // ================================================================
    static double computeEpsilon2(
        const SolverResult& coarse,
        const SolverResult& fine,
        int& iMax, int& jMax)
    {
        const int n = coarse.n;
        const int m = coarse.m;
        const int Ny = m + 1;
        const int Ny2 = 2 * m + 1;

        double eps2 = 0.0;
        iMax = 0; jMax = 0;

        for (int i = 0; i <= n; ++i) {
            for (int j = 0; j <= m; ++j) {
                const double v1 = coarse.v[i * Ny + j];
                const double v2 = fine.v[(2 * i) * Ny2 + (2 * j)];
                const double diff = std::abs(v1 - v2);
                if (diff > eps2) {
                    eps2 = diff;
                    iMax = i;
                    jMax = j;
                }
            }
        }
        return eps2;
    }
};