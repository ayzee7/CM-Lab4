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
    double residualNorm;   // норма невязки (max)
    double methodError;    // достигнутая точность итерационного метода
    double a, b, c, d;     // границы области

    // Сеточные функции (размер (n+1)*(m+1), индекс i*(m+1)+j)
    std::vector<double> v;   // численное решение
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
        double c2  = std::cos(pxy);
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
    //  ω_opt = 2 / (1 + sin(π h / (b-a)))  (приближённая формула)
    // ============================================================
    static double optimalOmega(int n, int m)
    {
        // Оценка спектрального радиуса матрицы Якоби
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

        int Nx = n + 1, Ny = m + 1;
        int total = Nx * Ny;

        // Лямбды правой части и граничных условий
        auto F  = isTest ? std::function<double(double,double)>(fTest)
                         : std::function<double(double,double)>(fMain);
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
                    // Линейная интерполяция граничных значений по x
                    double t = (double)i / n;
                    v[i * Ny + j] = (1.0 - t) * BC(0, j, a, yj)
                                  +        t  * BC(n, j, b, yj);
                }
            }
        }

        // --- Коэффициенты схемы ---
        double h2 = h * h, k2 = k * k;
        double denom = 2.0 / h2 + 2.0 / k2;   // главный диагональный коэффициент

        // Параметр (ω для МВР, 1.0 для Зейделя)
        double om = (method == Method::Seidel) ? 1.0 : omega;

        // --- Итерационный процесс ---
        double iterErr = 1e100;
        int    iter    = 0;

        while (iter < Nmax && iterErr > epsMet)
        {
            iterErr = 0.0;
            for (int i = 1; i < n; ++i) {
                double xi = a + i * h;
                for (int j = 1; j < m; ++j) {
                    double yj = c + j * k;
                    double fval = F(xi, yj);

                    // Стандартная 5-точечная схема
                    double rhs = fval
                               + (v[(i-1)*Ny+j] + v[(i+1)*Ny+j]) / h2
                               + (v[i*Ny+(j-1)] + v[i*Ny+(j+1)]) / k2;

                    double vNew = rhs / denom;
                    double old  = v[i * Ny + j];
                    double updated = old + om * (vNew - old);  // МВР / Зейдель

                    iterErr = std::max(iterErr, std::abs(updated - old));
                    v[i * Ny + j] = updated;
                }
            }
            ++iter;
        }

        res.iterDone   = iter;
        res.methodError = iterErr;
        res.v = v;

        // --- Норма невязки (max) ---
        double resNorm = 0.0;
        for (int i = 1; i < n; ++i) {
            double xi = a + i * h;
            for (int j = 1; j < m; ++j) {
                double yj = c + j * k;
                double Lv = (v[(i-1)*Ny+j] - 2*v[i*Ny+j] + v[(i+1)*Ny+j]) / h2
                           +(v[i*Ny+(j-1)] - 2*v[i*Ny+j] + v[i*Ny+(j+1)]) / k2;
                double R = F(xi, yj) + Lv;   // Δv + f
                resNorm = std::max(resNorm, std::abs(R));
            }
        }
        res.residualNorm = resNorm;

        // --- Погрешность для тестовой задачи ---
        res.epsilon1  = 0.0;
        res.iMaxErr   = 0;
        res.jMaxErr   = 0;
        if (isTest) {
            res.u.resize(total);
            for (int i = 0; i < Nx; ++i) {
                double xi = a + i * h;
                for (int j = 0; j < Ny; ++j) {
                    double yj = c + j * k;
                    double ex = uExact(xi, yj);
                    res.u[i * Ny + j] = ex;
                    double diff = std::abs(ex - v[i*Ny+j]);
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
