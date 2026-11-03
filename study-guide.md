Source: https://www.reddit.com/r/cpp/comments/1i8zct5/interview_questions_at_hft_firms_for_c_roles

OS. Re-read the OS textbook from your classes. Deep down, the data structures for paging, the interrupts when page fault occurs, how it is all handled. Same for data. What's the whole flow for an incoming packet. What is triggered in the OS, in the user space.
Templates. CRTP. Practice writing code where every possible condition or virtual call is replaced with a compile-time template parameter and a specific instanciation.


Constexpr everything.

Learn the nuances of r-value references, move semantics, universal references.

Review concurrency, lock-free structures, atomics. Consider false sharing, caching.

Make sure you know linux and dynamic loading. Learn the limitation of global initialization. Unlearn singletons.

Review determinism and how to make multi-threaded systems deterministic.


Implement std::unique_ptr and std::shared_ptr


I recently completed hiring loops for a few HFTs and Hedge funds for C++ developer positions. For background, I have 5.5 years of experience so I was interviewing for intermediate/senior level positions

Below are some of the type of rounds you can expect along with the typical topics in each one of them

Leetcode style

I would say only 1 or 2 rounds would be leetcode style where the focus is more on solving the problem and less on coding style/language features. In the end, they may ask you to few C++ related questions

OS/ Networks/ Computer Architecture

Q&A. Usually 2/3 rounds. Deep dive on topics like concurrency, multi threading, paging, virtual address translation, CPU cache hierarchies, atomics, TCP/UDP multicast

C++ implementation round

You are asked to implement a data structure in C++ like unique_ptr, shared_ptr, std::string (with and without SSO), fixed length string, std::vector, std::any. You can also be asked to implement a metaprogramming question e.g. compile time gcd, complie time vector and operations on it. It may also include a data structure for a custom problem that is described during the interview

C++ deepdive

Q&A round where the focus is on C++ questions like virtual functions, small string optimization, std::variant, difference between different type of casts, aliasing, compilations process, compiler optimizations, RAII, constexpr, etc

Domain specific round

Q&A round which covers domain specific questions pertaining to the role. Asked for intermediate and above level candidates
