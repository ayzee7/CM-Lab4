#pragma once
#define _USE_MATH_DEFINES

#include <vector>
#include <cmath>
#include <string>
#include <functional>
#include <algorithm>

// ============================================================
//  SolverResult – результат одного запуска МВР / Зейделя
// ============================================================
struct SolverResult
{
    int    n, m;           // число разбиений по x, y
    double h, k;           // шаги сетки
    double omega;          // параметр релаксации
    int    iterDone;       // фактическое число итераций
    double residualNorm;   // ||R(N)||∞ — невязка на финальном решении
    double residualNorm0;  // ||R(0)||∞ — невязка на начальном приближении
    double methodError;    // max|v^(k+1) − v^(k)| на последней итерации
    double effectiveError; // оценка реальной погрешности ≈ δ·ρ/(1−ρ)
    double rhoEstimate;    // апостериорная оценка спектрального радиуса
    double a, b, c, d;     // границы области

    // Сеточные функции (размер (n+1)*(m+1), индекс i*(m+1)+j)
    std::vector<double> v;   // численное решение
    std::vector<double> v0;  // начальное приближение
    std::vector<double> u;   // точное решение (только для тестовой задачи)

    // Погрешность / точность
    double epsilon1;       // max|u*-v| (тестовая задача)
    double epsilon2;       // max|v-v2| (основная задача, заполняется снаружи)

    // Узел максимального отклонения
    int iMaxErr, jMaxErr;
};

// ============================================================
//  PoissonSolver
// ============================================================
class PoissonSolver
{
public:
    // ---------- параметры задачи ----------
    double a = 0, b = 1, c = 0, d = 1;

    // ---------- параметры метода ----------
    enum class Method { SOR, Seidel };  // МВР или Зейдель (ω=1)

    // ============================================================
    //  Тестовая задача (вариант 1)
    //  u*(x,y) = exp(sin^2(pi*x*y))
    //  f*(x,y) = -Δu* (вычисляется аналитически)
    //  Граничные условия берутся из u*
    // ============================================================

    static double uExact(double x, double y)
    {
        double s = std::sin(M_PI * x * y);
        return std::exp(s * s);
    }

    // Лапласиан u* вычисляется аналитически:
    // u = exp(sin^2(π x y))
    // du/dx = u * 2 sin(πxy) cos(πxy) * πy = u * sin(2πxy) * πy
    // d²u/dx² = ...  (полная формула ниже)
    static double fTest(double x, double y)
    {
        // -Δu* (правая часть тестовой задачи)
        double pxy = M_PI * x * y;
        double s   = std::sin(pxy);
        double u   = std::exp(s * s);

        // ∂u/∂x = u * π y * sin(2πxy)
        // ∂²u/∂x² = u * [ (πy sin(2πxy))^2 + 2π²y² cos(2πxy) ]
        double sin2 = std::sin(2.0 * pxy);
        double cos2 = std::cos(2.0 * pxy);

        double dxx = u * (M_PI * M_PI * y * y * sin2 * sin2 +
                          2.0 * M_PI * M_PI * y * y * cos2);
        double dyy = u * (M_PI * M_PI * x * x * sin2 * sin2 +
                          2.0 * M_PI * M_PI * x * x * cos2);

        return -(dxx + dyy);   // -Δu*
    }

    // Правая часть ОСНОВНОЙ задачи (вариант 1): f(x,y) = sin^2(πxy)
    static double fMain(double x, double y)
    {
        double s = std::sin(M_PI * x * y);
        return s * s;
    }

    // Граничные условия основной задачи (вариант 1)
    // μ1(y)=sin(πy), μ2(y)=sin(πy), μ3(x)=x-x^2, μ4(x)=x-x^2
    static double mu1(double y) { return std::sin(M_PI * y); }
    static double mu2(double y) { return std::sin(M_PI * y); }
    static double mu3(double x) { return x - x * x; }
    static double mu4(double x) { return x - x * x; }

    // ============================================================
    //  Оптимальный параметр МВР для прямоугольной области
    //  ω_opt = 2 / (1 + sqrt(1 − ρ²))
    // ============================================================
    static double optimalOmega(int n, int m)
    {
        double rho = 0.5 * (std::cos(M_PI / n) + std::cos(M_PI / m));
        return 2.0 / (1.0 + std::sqrt(1.0 - rho * rho));
    }

    // ============================================================
    //  Основной решатель
    //  isTest  – тестовая (true) или основная (false) задача
    //  method  – SOR (МВР) или Seidel (Зейдель, ω=1)
    // ============================================================
    SolverResult solve(
        int    n,          // число разбиений по x
        int    m,          // число разбиений по y
        double omega,      // параметр релаксации (для Зейделя передаётся 1.0)
        double epsMet,     // критерий остановки по точности
        int    Nmax,       // максимальное число итераций
        bool   isTest,
        Method method = Method::SOR) const
    {
        SolverResult res;
        res.n = n; res.m = m;
        res.a = a; res.b = b; res.c = c; res.d = d;
        res.omega = omega;

        double h = (b - a) / n;
        double k = (d - c) / m;
        res.h = h; res.k = k;

        const int Nx = n + 1, Ny = m + 1;
        const int total = Nx * Ny;

        auto BC = [&](int i, int j, double xi, double yj) -> double {
            if (isTest) return uExact(xi, yj);
            if (i == 0)  return mu1(yj);
            if (i == n)  return mu2(yj);
            if (j == 0)  return mu3(xi);
            if (j == m)  return mu4(xi);
            return 0.0;
        };

        // --- Начальное приближение: линейная интерполяция по x ---
        std::vector<double> v(total, 0.0);
        for (int i = 0; i < Nx; ++i) {
            double xi = a + i * h;
            for (int j = 0; j < Ny; ++j) {
                double yj = c + j * k;
                if (i == 0 || i == n || j == 0 || j == m) {
                    v[i * Ny + j] = BC(i, j, xi, yj);
                } else {
                    double t = (double)i / n;
                    v[i * Ny + j] = (1.0 - t) * BC(0, j, a, yj)
                                  +        t  * BC(n, j, b, yj);
                }
            }
        }
        res.v0 = v;

        // --- Предвычисление правой части во внутренних узлах ---
        // Снимает 3–5 трансцендентных вызовов на узел в каждой итерации.
        std::vector<double> fGrid(total, 0.0);
        for (int i = 1; i < n; ++i) {
            double xi = a + i * h;
            for (int j = 1; j < m; ++j) {
                double yj = c + j * k;
                fGrid[i * Ny + j] = isTest ? fTest(xi, yj) : fMain(xi, yj);
            }
        }

        // --- Коэффициенты схемы (умножения вместо делений) ---
        const double invH2 = 1.0 / (h * h);
        const double invK2 = 1.0 / (k * k);
        const double invDenom = 1.0 / (2.0 * (invH2 + invK2));
        const double om = (method == Method::Seidel) ? 1.0 : omega;

        // --- Невязка ||F + Δv||∞ во внутренних узлах ---
        auto residualMax = [&](const std::vector<double>& w) -> double {
            double rn = 0.0;
            for (int i = 1; i < n; ++i) {
                for (int j = 1; j < m; ++j) {
                    int idx = i * Ny + j;
                    double Lv = (w[idx - Ny] - 2.0 * w[idx] + w[idx + Ny]) * invH2
                              + (w[idx - 1]  - 2.0 * w[idx] + w[idx + 1])  * invK2;
                    double R = fGrid[idx] + Lv;
                    double ar = std::abs(R);
                    if (ar > rn) rn = ar;
                }
            }
            return rn;
        };

        // Невязка на начальном приближении (для справки)
        res.residualNorm0 = residualMax(v);

        // --- Итерационный процесс с адаптивной оценкой сходимости ---
        // δ_k = max|v^(k) − v^(k−1)|.  Если ρ — асимптотический фактор
        // подавления невязки, то ||v − v*||∞ ≈ δ · ρ / (1 − ρ).
        // При ρ → 1 (большие сетки) "сырой" δ занижает реальную ошибку
        // на (1−ρ)⁻¹ раз — поэтому останавливаемся по effErr.
        double iterErr = 0.0;
        double prevErr = 0.0;
        double rhoEst  = 0.0;
        double effErr  = 1e100;
        int    iter    = 0;
        const int rhoWarmup = 10;

        while (iter < Nmax && effErr > epsMet)
        {
            iterErr = 0.0;
            for (int i = 1; i < n; ++i) {
                double rowMax = 0.0;
                int row = i * Ny;
                for (int j = 1; j < m; ++j) {
                    int idx = row + j;
                    double rhs = fGrid[idx]
                               + (v[idx - Ny] + v[idx + Ny]) * invH2
                               + (v[idx - 1]  + v[idx + 1])  * invK2;
                    double vNew    = rhs * invDenom;
                    double old     = v[idx];
                    double updated = old + om * (vNew - old);   // МВР / Зейдель
                    double d = std::abs(updated - old);
                    if (d > rowMax) rowMax = d;
                    v[idx] = updated;
                }
                if (rowMax > iterErr) iterErr = rowMax;
            }
            ++iter;

            // Апостериорная оценка ρ с экспоненциальным сглаживанием
            if (iter > rhoWarmup && prevErr > 1e-30 && iterErr > 0.0) {
                double r = iterErr / prevErr;
                if (r > 0.0 && r < 1.0)
                    rhoEst = (rhoEst == 0.0) ? r : 0.9 * rhoEst + 0.1 * r;
            }
            prevErr = iterErr;

            // Реалистичная оценка погрешности
            if (rhoEst > 1e-3 && rhoEst < 1.0)
                effErr = iterErr * rhoEst / (1.0 - rhoEst);
            else
                effErr = iterErr;
        }

        res.iterDone       = iter;
        res.methodError    = iterErr;
        res.effectiveError = effErr;
        res.rhoEstimate    = rhoEst;
        res.v = v;

        // --- Итоговая невязка ---
        res.residualNorm = residualMax(v);

        // --- Погрешность для тестовой задачи ---
        res.epsilon1 = 0.0;
        res.iMaxErr  = 0;
        res.jMaxErr  = 0;
        if (isTest) {
            res.u.resize(total);
            for (int i = 0; i < Nx; ++i) {
                double xi = a + i * h;
                for (int j = 0; j < Ny; ++j) {
                    double yj = c + j * k;
                    double ex = uExact(xi, yj);
                    res.u[i * Ny + j] = ex;
                    double diff = std::abs(ex - v[i * Ny + j]);
                    if (diff > res.epsilon1) {
                        res.epsilon1 = diff;
                        res.iMaxErr  = i;
                        res.jMaxErr  = j;
                    }
                }
            }
        }
        res.epsilon2 = 0.0;

        return res;
    }

    // ============================================================
    //  Оценка точности основной задачи: ε2 = max|v - v2| в общих узлах
    // ============================================================
    static double computeEpsilon2(
        const SolverResult& coarse,
        const SolverResult& fine,
        int& iMax, int& jMax)
    {
        int n  = coarse.n, m  = coarse.m;
        int Ny = m + 1;

        double eps2 = 0.0;
        iMax = 0; jMax = 0;

        // Узлы грубой сетки совпадают с узлами 0,2,4,... тонкой
        for (int i = 0; i <= n; ++i) {
            for (int j = 0; j <= m; ++j) {
                double v1 = coarse.v[i * Ny + j];
                double v2 = fine.v[(2*i) * (2*m+1) + (2*j)];
                double diff = std::abs(v1 - v2);
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
