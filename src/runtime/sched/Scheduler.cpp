#include "Scheduler.hpp"

thread_local Scheduler* g_CurrentScheduler = nullptr;

#if defined(__x86_64__) || defined(_M_X64)
#if defined(_WIN32) || defined(_WIN64)
__attribute__((naked)) void Scheduler::switchContext(ExecutionContext* from, ExecutionContext* to) {
    __asm__ (
        "movq %rsp, 0(%rcx)\n\t"
        "movq %rbp, 8(%rcx)\n\t"
        "movq %rbx, 16(%rcx)\n\t"
        "movq %r12, 24(%rcx)\n\t"
        "movq %r13, 32(%rcx)\n\t"
        "movq %r14, 40(%rcx)\n\t"
        "movq %r15, 48(%rcx)\n\t"
        "leaq 1f(%rip), %rax\n\t"
        "movq %rax, 56(%rcx)\n\t"
        "movdqa %xmm0, 64(%rcx)\n\t"
        "movdqa %xmm1, 80(%rcx)\n\t"
        "movdqa %xmm2, 96(%rcx)\n\t"
        "movdqa %xmm3, 112(%rcx)\n\t"
        "movdqa %xmm4, 128(%rcx)\n\t"
        "movdqa %xmm5, 144(%rcx)\n\t"
        "movdqa %xmm6, 160(%rcx)\n\t"
        "movdqa %xmm7, 176(%rcx)\n\t"
        "movdqa %xmm8, 192(%rcx)\n\t"
        "movdqa %xmm9, 208(%rcx)\n\t"
        "movdqa %xmm10, 224(%rcx)\n\t"
        "movdqa %xmm11, 240(%rcx)\n\t"
        "movdqa %xmm12, 256(%rcx)\n\t"
        "movdqa %xmm13, 272(%rcx)\n\t"
        "movdqa %xmm14, 288(%rcx)\n\t"
        "movdqa %xmm15, 304(%rcx)\n\t"

        "movq 0(%rdx), %rsp\n\t"
        "movq 8(%rdx), %rbp\n\t"
        "movq 16(%rdx), %rbx\n\t"
        "movq 24(%rdx), %r12\n\t"
        "movq 32(%rdx), %r13\n\t"
        "movq 40(%rdx), %r14\n\t"
        "movq 48(%rdx), %r15\n\t"
        "movq 56(%rdx), %rax\n\t"
        "movdqa 64(%rdx), %xmm0\n\t"
        "movdqa 80(%rdx), %xmm1\n\t"
        "movdqa 96(%rdx), %xmm2\n\t"
        "movdqa 112(%rdx), %xmm3\n\t"
        "movdqa 128(%rdx), %xmm4\n\t"
        "movdqa 144(%rdx), %xmm5\n\t"
        "movdqa 160(%rdx), %xmm6\n\t"
        "movdqa 176(%rdx), %xmm7\n\t"
        "movdqa 192(%rdx), %xmm8\n\t"
        "movdqa 208(%rdx), %xmm9\n\t"
        "movdqa 224(%rdx), %xmm10\n\t"
        "movdqa 240(%rdx), %xmm11\n\t"
        "movdqa 256(%rdx), %xmm12\n\t"
        "movdqa 272(%rdx), %xmm13\n\t"
        "movdqa 288(%rdx), %xmm14\n\t"
        "movdqa 304(%rdx), %xmm15\n\t"

        "jmpq *%rax\n\t"
        "1:\n\t"
        "ret\n\t"
    );
}
#else
__attribute__((naked)) void Scheduler::switchContext(ExecutionContext* from, ExecutionContext* to) {
    __asm__ (
        "movq %rsp, 0(%rdi)\n\t"
        "movq %rbp, 8(%rdi)\n\t"
        "movq %rbx, 16(%rdi)\n\t"
        "movq %r12, 24(%rdi)\n\t"
        "movq %r13, 32(%rdi)\n\t"
        "movq %r14, 40(%rdi)\n\t"
        "movq %r15, 48(%rdi)\n\t"
        "leaq 1f(%rip), %rax\n\t"
        "movq %rax, 56(%rdi)\n\t"
        "movdqa %xmm0, 64(%rdi)\n\t"
        "movdqa %xmm1, 80(%rdi)\n\t"
        "movdqa %xmm2, 96(%rdi)\n\t"
        "movdqa %xmm3, 112(%rdi)\n\t"
        "movdqa %xmm4, 128(%rdi)\n\t"
        "movdqa %xmm5, 144(%rdi)\n\t"
        "movdqa %xmm6, 160(%rdi)\n\t"
        "movdqa %xmm7, 176(%rdi)\n\t"
        "movdqa %xmm8, 192(%rdi)\n\t"
        "movdqa %xmm9, 208(%rdi)\n\t"
        "movdqa %xmm10, 224(%rdi)\n\t"
        "movdqa %xmm11, 240(%rdi)\n\t"
        "movdqa %xmm12, 256(%rdi)\n\t"
        "movdqa %xmm13, 272(%rdi)\n\t"
        "movdqa %xmm14, 288(%rdi)\n\t"
        "movdqa %xmm15, 304(%rdi)\n\t"

        "movq 0(%rsi), %rsp\n\t"
        "movq 8(%rsi), %rbp\n\t"
        "movq 16(%rsi), %rbx\n\t"
        "movq 24(%rsi), %r12\n\t"
        "movq 32(%rsi), %r13\n\t"
        "movq 40(%rsi), %r14\n\t"
        "movq 48(%rsi), %r15\n\t"
        "movq 56(%rsi), %rax\n\t"
        "movdqa 64(%rsi), %xmm0\n\t"
        "movdqa 80(%rsi), %xmm1\n\t"
        "movdqa 96(%rsi), %xmm2\n\t"
        "movdqa 112(%rsi), %xmm3\n\t"
        "movdqa 128(%rsi), %xmm4\n\t"
        "movdqa 144(%rsi), %xmm5\n\t"
        "movdqa 160(%rsi), %xmm6\n\t"
        "movdqa 176(%rsi), %xmm7\n\t"
        "movdqa 192(%rsi), %xmm8\n\t"
        "movdqa 208(%rsi), %xmm9\n\t"
        "movdqa 224(%rsi), %xmm10\n\t"
        "movdqa 240(%rsi), %xmm11\n\t"
        "movdqa 256(%rsi), %xmm12\n\t"
        "movdqa 272(%rsi), %xmm13\n\t"
        "movdqa 288(%rsi), %xmm14\n\t"
        "movdqa 304(%rsi), %xmm15\n\t"

        "jmpq *%rax\n\t"
        "1:\n\t"
        "ret\n\t"
    );
}
#endif
#elif defined(__i386__) || defined(_M_IX86)
__attribute__((naked)) void Scheduler::switchContext(ExecutionContext* from, ExecutionContext* to) {
    __asm__ (
        "movl 4(%esp), %eax\n\t"
        "movl %esp, 0(%eax)\n\t"
        "movl %ebp, 4(%eax)\n\t"
        "movl %ebx, 8(%eax)\n\t"
        "movl %esi, 12(%eax)\n\t"
        "movl %edi, 16(%eax)\n\t"
        "leal 1f, %ecx\n\t"
        "movl %ecx, 20(%eax)\n\t"

        "movl 8(%esp), %edx\n\t"
        "movl 0(%edx), %esp\n\t"
        "movl 4(%edx), %ebp\n\t"
        "movl 8(%edx), %ebx\n\t"
        "movl 12(%edx), %esi\n\t"
        "movl 16(%edx), %edi\n\t"
        "movl 20(%edx), %ecx\n\t"

        "jmpl *%ecx\n\t"
        "1:\n\t"
        "ret\n\t"
    );
}
#else
void Scheduler::switchContext(ExecutionContext* from, ExecutionContext* to) {
    (void)from; (void)to;
}
#endif

void Scheduler::workerLoop(size_t workerId) {
    auto myQueue = pool.getQueue(workerId);

    while (running.load(std::memory_order_relaxed)) {
        Task* task = myQueue->pop();

        if (!task) {
            task = pool.stealTask(workerId);
        }

        if (task) {
            task->state = TaskState::Running;
            if (task->entryFunc) {
                task->entryFunc(task->argument);
            }
            task->state = TaskState::Dead;
            delete task;
        } else {
            std::this_thread::yield();
        }
    }
}

void Scheduler::start() {
    running.store(true, std::memory_order_relaxed);
    for (size_t i = 0; i < numThreads; ++i) {
        workers.emplace_back(&Scheduler::workerLoop, this, i);
    }
}

void Scheduler::stop() {
    if (running.exchange(false, std::memory_order_relaxed)) {
        for (auto& w : workers) {
            if (w.joinable()) w.join();
        }
        workers.clear();
    }
}

void Scheduler::yield(size_t workerId) {
    std::this_thread::yield();
}