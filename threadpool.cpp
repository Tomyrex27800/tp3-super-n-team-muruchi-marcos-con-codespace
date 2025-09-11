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

ThreadPool::ThreadPool() : stop_(false)
{
    // creacion de 5 worker threads
    for (size_t i = 0; i < 5; i++) {
        threads_.emplace_back([this] {
            while (true)
            {
                function<void()> task;
                {
                    // se bloquea la cola asi se pueden compartir datos de forma segura
                    unique_lock<mutex> lock(queue_mutex_);

                    // esperar a una tarea a ejecutar o a que se detenga el pool
                    cv_.wait(lock, [this] {
                        return !tasks_.empty() || stop_;
                    });

                    // salir del thread si el pool esta detenido y no hay tareas
                    if (stop_ && tasks_.empty()) {
                        return;
                    }

                    // agarrar la siguiente tarea de la cola de tareas
                    // la funcion "move", segun leí, le indica al compilador que la variable a la que se accede se mueve tal cual
                    task = move(tasks_.front());
                    tasks_.pop();
                }

                task();
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

    // se joinean todos los workers para asegurar que hayan completado sus tareas
    for (auto& thread : threads_) {
        thread.join();
    }
}

void ThreadPool::enqueue(function<void()> task) {
    {
        unique_lock<mutex> lock(queue_mutex_);
        tasks_.emplace(move(task));
    }
    cv_.notify_one();
}
