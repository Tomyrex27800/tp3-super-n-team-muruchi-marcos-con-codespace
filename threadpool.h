#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class ThreadPool {
public:
    // inicializacion de pool de threads con 5 worker threads
    ThreadPool();

    // destructor, para el pool
    ~ThreadPool();

    // encola conexiones nuevas
    bool enqueue(std::function<void()> task);

private:
    // vector de workers
    std::vector<std::thread> threads_;

    // cola de conexiones
    std::queue<std::function<void()>> tasks_;

    // mutex para el acceso a datos compartidos
    std::mutex queue_mutex_;

    // cv para avisar de cambios en el estado de la cola de conexiones
    std::condition_variable cv_;

    // flag para indicar si el pool tiene que parar o no
    bool stop_;

    int current_connections_;
};

#endif
