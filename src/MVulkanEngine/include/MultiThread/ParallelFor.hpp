#ifndef PARALLEL_FOR_HPP
#define PARALLEL_FOR_HPP

#include "MultiThread/Task.hpp"
#include <thread>   // �ṩ std::thread
#include <algorithm> // �ṩ std::for_each
#include <vector>

template <typename Iterator, typename Func>
void ParallelFor(Iterator first, Iterator last, const Func& f, size_t num_threads = 0) {
    // ����Ԫ������
    const size_t total_size = std::distance(first, last);
    if (total_size == 0) return;

    // �Զ�ȷ���߳���
    if (num_threads == 0) {
        num_threads = std::thread::hardware_concurrency();
        if (num_threads == 0) num_threads = 1;
    }
    num_threads = std::min(num_threads, total_size);

    // ����ÿ���̴߳����Ԫ������
    const size_t chunk_size = total_size / num_threads;

    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    Iterator start = first;
    for (size_t i = 0; i < num_threads - 1; ++i) {
        Iterator end = start;
        std::advance(end, chunk_size);

        threads.emplace_back([start, end, &f] {
            std::for_each(start, end, f);
            });

        start = end;
    }

    // ���̴߳������һ����
    std::for_each(start, last, f);

    // �ȴ������߳����
    for (auto& thread : threads) {
        thread.join();
    }
}

#endif