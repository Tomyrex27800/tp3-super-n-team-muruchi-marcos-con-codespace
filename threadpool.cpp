#include "threadpool.h"

#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

using namespace std;

// este thread pool fue copiado a mano y a conciencia desde https://www.geeksforgeeks.org/cpp/thread-pool-in-cpp/
// JURO QUE IJNTENTE ENTENDERLO

ThreadPool::ThreadPool() : stop_(false), current_connections_(0)
{
    // creacion de 5 worker threads
    for (size_t i = 0; i < 5; i++) {
        threads_.emplace_back([this] {
            while (true)
            {
                function<void()> task;
                {
                    // se bloquea la cola asi se pueden cambiar datos de forma segura
                    unique_lock<mutex> lock(queue_mutex_);

                    // esperar a una conexión o a que se detenga el pool
                    cv_.wait(lock, [this] {
                        return !tasks_.empty() || stop_;
                    });

                    // salir del thread si el pool esta detenido y no hay conexiones
                    if (stop_ && tasks_.empty()) {
                        return;
                    }

                    // agarrar la siguiente conexión de la cola de conexiones
                    // la funcion "move", segun leí, le indica al compilador que la variable a la que se accede se mueve tal cual
                    task = move(tasks_.front());
                    tasks_.pop();
                    current_connections_ += 1;
                }

                task();
                unique_lock<mutex> lock(queue_mutex_);
                current_connections_ -= 1;
            }
        });
    }
}

ThreadPool::~ThreadPool()
{
    {
        // bloquea la cola para actualizar el flag stop de forma segura
        unique_lock<mutex> lock(queue_mutex_);
        stop_ = true;
    }

    // notificar todos los threads;
    cv_.notify_all();

    // se joinean todos los workers para asegurar que hayan cerrado sus conexiones
    for (auto& thread : threads_) {
        thread.join();
    }
}

bool ThreadPool::enqueue(function<void()> task) {
    {
        unique_lock<mutex> lock(queue_mutex_);

        // si existen 5 conexiones, la conexión se rechaza.
        if (current_connections_ >= 5) {
            return false;
        }

        tasks_.emplace(move(task));
    }
    cv_.notify_one();

    return true;
}
