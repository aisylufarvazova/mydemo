#include <vector>
#include <cstdio>
#include <set>
#include <list>
#include <map>
#include <algorithm>
#include <iterator>
#include <iostream>
#include <functional>
#include <memory>
#include <stdexcept>
#include <utility>
#include <fstream>

template <class T, class Compare = std::less<T>>
class Heap {
public:
    using IndexChangeObserver =
        std::function<void(const T& element, size_t new_element_index)>;

    static constexpr size_t kNullIndex = static_cast<size_t>(-1);

    explicit Heap(
        Compare compare = Compare(),
        IndexChangeObserver index_change_observer = IndexChangeObserver());

    size_t push(const T& value);
    void erase(size_t index);
    const T& top() const;
    void pop();
    size_t size() const;
    bool empty() const;

private:
    IndexChangeObserver index_change_observer_;
    Compare compare_;
    std::vector<T> elements_;

    size_t Parent(size_t index) const;
    size_t LeftSon(size_t index) const;
    size_t RightSon(size_t index) const;

    bool CompareElements(size_t first_index, size_t second_index) const;
    void NotifyIndexChange(const T& element, size_t new_element_index);
    void SwapElements(size_t first_index, size_t second_index);
    size_t SiftUp(size_t index);
    void SiftDown(size_t index);
};

struct MemorySegment {
    int left;
    int right;
    size_t heap_index;

    MemorySegment(int left, int right);
    size_t Size() const;
    MemorySegment Unite(const MemorySegment& other) const;
};

using MemorySegmentIterator = std::list<MemorySegment>::iterator;
using MemorySegmentConstIterator = std::list<MemorySegment>::const_iterator;


struct MemorySegmentSizeCompare {
    bool operator() (MemorySegmentIterator first,
                     MemorySegmentIterator second) const;
};


using MemorySegmentHeap = Heap<MemorySegmentIterator, MemorySegmentSizeCompare>;


struct MemorySegmentsHeapObserver {
    void operator() (MemorySegmentIterator segment, size_t new_index) const;
};


class MemoryManager {
public:
    using Iterator = MemorySegmentIterator;
    using ConstIterator = MemorySegmentConstIterator;

    explicit MemoryManager(size_t memory_size);
    Iterator Allocate(size_t size);
    void Free(Iterator position);
    Iterator end();
    ConstIterator end() const;

private:
    MemorySegmentHeap free_memory_segments_;
    std::list<MemorySegment> memory_segments_;

    void AppendIfFree(Iterator remaining, Iterator appending);
};


size_t ReadMemorySize(std::istream& stream = std::cin);

struct AllocationQuery {
    size_t allocation_size;
};

struct FreeQuery {
    int allocation_query_index;
};


class MemoryManagerQuery {
public:
    explicit MemoryManagerQuery(AllocationQuery allocation_query);
    explicit MemoryManagerQuery(FreeQuery free_query);

    const AllocationQuery* AsAllocationQuery() const;
    const FreeQuery* AsFreeQuery() const;

private:
    class AbstractQuery {
    public:
        virtual ~AbstractQuery() {
        }

    protected:
        AbstractQuery() {
        }
    };

    template <typename T>
    struct ConcreteQuery : public AbstractQuery {
        T body;

        explicit ConcreteQuery(T body_)
            : body(std::move(body_)) {}
    };

    std::unique_ptr<AbstractQuery> query_;
};

std::vector<MemoryManagerQuery> ReadMemoryManagerQueries(std::istream& stream = std::cin);

struct MemoryManagerAllocationResponse {
    bool success;
    size_t position;
};


MemoryManagerAllocationResponse MakeSuccessfulAllocation(size_t position);

MemoryManagerAllocationResponse MakeFailedAllocation();

std::vector<MemoryManagerAllocationResponse> RunMemoryManager(
    size_t memory_size,
    const std::vector<MemoryManagerQuery>& queries);

void OutputMemoryManagerResponses(
    const std::vector<MemoryManagerAllocationResponse>& responses,
    std::ostream& ostream = std::cout);


template<typename T>
std::ostream& operator << (std::ostream& os, const std::vector<T> & object);
std::ostream& operator << (std::ostream& os, const MemorySegment & block);
std::ostream& operator << (std::ostream& os, const std::list<MemorySegment> & object);



int main() {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);

    std::istream& input_stream = std::cin;
    std::ostream& output_stream = std::cout;

    const size_t memory_size = ReadMemorySize(input_stream);
    const std::vector<MemoryManagerQuery> queries =
        ReadMemoryManagerQueries(input_stream);

    const std::vector<MemoryManagerAllocationResponse> responses =
        RunMemoryManager(memory_size, queries);

    OutputMemoryManagerResponses(responses, output_stream);

    return 0;
}



/* Определяем операторы вывода */

template<typename T>
std::ostream& operator << (std::ostream& os, const std::vector<T>& object) {
    os << "{";
    for (size_t i = 0; i < object.size(); ++i) {
        os << *(object[i]);
        if (i + 1 < object.size()) {
            os << ",";
        }
    }
    return os << "}";
}

std::ostream& operator << (std::ostream& os, const MemorySegment & block) {
    os << "[" << block.left << ", ";
    os << (block.heap_index != MemorySegmentHeap::kNullIndex ? "free" : "used");
    if (block.heap_index != MemorySegmentHeap::kNullIndex) {
        os << "{" << block.heap_index << "}";
    }
    os << ", " << block.right << "]";
    return os;
}

std::ostream& operator << (std::ostream& os, const std::list<MemorySegment>& object) {
    for (MemorySegmentConstIterator i = object.begin(); i != object.end(); ++i) {
        os << (*i) << " -> ";
    }
    return os << "NULL";
}

/* Закончили определять операторы вывода */

/* Определяем методы класса Heap */

template<class T, class Compare>
Heap<T, Compare>::Heap(Compare compare, IndexChangeObserver index_change_observer)
    : compare_(compare), index_change_observer_(index_change_observer) {}

template<class T, class Compare>
size_t Heap<T, Compare>::Parent(size_t index) const {
    return (index - 1) / 2;
}

template<class T, class Compare>
size_t Heap<T, Compare>::LeftSon(size_t index) const {
    return 2 * index + 1 < elements_.size() ? 2 * index + 1 : MemorySegmentHeap::kNullIndex;
}

template<class T, class Compare>
size_t Heap<T, Compare>::RightSon(size_t index) const {
    return 2 * index + 2 < elements_.size() ? 2 * index + 2 : MemorySegmentHeap::kNullIndex;
}

template<class T, class Compare>
bool Heap<T, Compare>::CompareElements(size_t first_index, size_t second_index) const {
    return compare_(elements_[first_index], elements_[second_index]);
}

template<class T, class Compare>
void Heap<T, Compare>::NotifyIndexChange(const T& element, size_t new_element_index) {
    index_change_observer_(element, new_element_index);
}

template<class T, class Compare>
void Heap<T, Compare>::SwapElements(size_t first_index, size_t second_index) {
    std::swap(elements_[first_index], elements_[second_index]);
    NotifyIndexChange(elements_[first_index], first_index);
    NotifyIndexChange(elements_[second_index], second_index);
}

template<class T, class Compare>
size_t Heap<T, Compare>::SiftUp(size_t index) {
    while (index > 0) {
        if (CompareElements(index, Parent(index))) {
            SwapElements(index, Parent(index));
        } else {
            return index;
        }
        index = Parent(index);
    }
    return index;
}

template<class T, class Compare>
void Heap<T, Compare>::SiftDown(size_t index) {
    while (LeftSon(index) != MemorySegmentHeap::kNullIndex) {
        size_t swap_index = MemorySegmentHeap::kNullIndex;
        if (RightSon(index) == MemorySegmentHeap::kNullIndex) {
            if (CompareElements(LeftSon(index), index)) {
                swap_index = LeftSon(index);
            }
        } else {
            if (CompareElements(LeftSon(index), RightSon(index))) {
                if (CompareElements(LeftSon(index), index)) {
                    swap_index = LeftSon(index);
                }
            } else if (CompareElements(RightSon(index), index)) {
                    swap_index = RightSon(index);
            }
        }
        if (swap_index != MemorySegmentHeap::kNullIndex) {
            SwapElements(swap_index, index);
            index = swap_index;
        } else {
            return;
        }
    }
}

template<class T, class Compare>
size_t Heap<T, Compare>::push(const T& value) {
    elements_.push_back(value);
    NotifyIndexChange(value, elements_.size() - 1);
    size_t new_index = SiftUp(elements_.size() - 1);
    return new_index;
}

template<class T, class Compare>
void Heap<T, Compare>::erase(size_t index) {
    SwapElements(index, elements_.size() - 1);
    NotifyIndexChange(elements_.back(), MemorySegmentHeap::kNullIndex);
    elements_.pop_back();
    const size_t new_index = SiftUp(index);
    SiftDown(new_index);
}

template<class T, class Compare>
const T& Heap<T, Compare>::top() const {
    return elements_[0];
}

template<class T, class Compare>
void Heap<T, Compare>::pop() {
    erase(0);
}

template<class T, class Compare>
size_t Heap<T, Compare>::size() const {
    return elements_.size();
}

template<class T, class Compare>
bool Heap<T, Compare>::empty() const {
    return elements_.size() == 0;
}

/* Закончили определять методы класса Heap */

/* Определяем методы структуры MemorySegment */

MemorySegment::MemorySegment(int left, int right)
    : left(left), right(right), heap_index(MemorySegmentHeap::kNullIndex) {}

size_t MemorySegment::Size() const {
    return right - left + 1;
}

MemorySegment MemorySegment::Unite(const MemorySegment& other) const {
    MemorySegment united_segment(std::min(left, other.left),
                                 std::max(right, other.right));
    return united_segment;
}

/* Закончили определять методы структуры MemorySegment */

/* Определяем операторы структур MemorySegmentSizeCompare и MemorySegmentsHeapObserver */

bool MemorySegmentSizeCompare::operator() (MemorySegmentIterator first,
                                           MemorySegmentIterator second) const {
    return second->Size() < first->Size() ||
           (first->Size() == second->Size() && first->left < second->left);
}

void MemorySegmentsHeapObserver::operator() (MemorySegmentIterator segment, 
                                             size_t new_index) const {
    segment->heap_index = new_index;
}

/* Закончили определять операторы структур MemorySegmentSizeCompare и MemorySegmentsHeapObserver */

/* Определяем методы класса MemoryManager */

MemoryManager::MemoryManager(size_t memory_size) {
    free_memory_segments_ = MemorySegmentHeap({}, MemorySegmentsHeapObserver());
    MemorySegment first_segment(1, memory_size);
    memory_segments_.push_back(first_segment);
    free_memory_segments_.push(memory_segments_.begin());
}

MemoryManager::Iterator MemoryManager::Allocate(size_t size) {
    if (free_memory_segments_.empty()) {
        return end();
    }
    MemoryManager::Iterator filled_segment = free_memory_segments_.top();
    if (filled_segment->Size() < size) {
        return end();
    } 
    if (filled_segment->Size() == size) {
        free_memory_segments_.pop();
    } else {
        free_memory_segments_.pop();
        memory_segments_.insert(filled_segment, *filled_segment);
        filled_segment->left = filled_segment->left + size;
        std::prev(filled_segment, 1)->right = filled_segment->left - 1;
        free_memory_segments_.push(filled_segment);
        --filled_segment;
    }
    return filled_segment;
}

void MemoryManager::Free(MemoryManager::Iterator position) {
    if (std::next(position, 1) != end()) {
        AppendIfFree(position, std::next(position, 1));
    }
    if (std::prev(position, 1) != end()) {
        AppendIfFree(position, std::prev(position, 1));
    }
    free_memory_segments_.push(position);
}

MemoryManager::Iterator MemoryManager::end() {
    return memory_segments_.end();
}

MemoryManager::ConstIterator MemoryManager::end() const {
    return  memory_segments_.cend();
}

void MemoryManager::AppendIfFree(MemoryManager::Iterator remaining,
                                 MemoryManager::Iterator appending) {
    if (appending->heap_index != MemorySegmentHeap::kNullIndex) {
        free_memory_segments_.erase(appending->heap_index);
        *remaining = remaining->Unite(*appending);
        memory_segments_.erase(appending);
    }
}

/* Закончили определять методы класса MemorySegment */

/* Определяем методы класса MemoryManagerQuery */

MemoryManagerQuery::MemoryManagerQuery(AllocationQuery allocation_query)
    : query_(new ConcreteQuery<AllocationQuery>(allocation_query)) {
}

MemoryManagerQuery::MemoryManagerQuery(FreeQuery free_query)
    : query_(new ConcreteQuery<FreeQuery>(free_query)) {
}

const AllocationQuery* MemoryManagerQuery::AsAllocationQuery() const {
    const ConcreteQuery<AllocationQuery>* allocation_query_wrapper =
        dynamic_cast<ConcreteQuery<AllocationQuery>*>(query_.get());
    return allocation_query_wrapper ? &(allocation_query_wrapper->body) : nullptr; 
}

const FreeQuery* MemoryManagerQuery::AsFreeQuery() const {
    const ConcreteQuery<FreeQuery>* free_query_wrapper = 
        dynamic_cast<ConcreteQuery<FreeQuery>*>(query_.get());
    return free_query_wrapper ? &(free_query_wrapper->body) : nullptr;
}

/* Закончили определять методы класса MemoryManagerQuery */

/* Определяем функции ввода/вывода */

size_t ReadMemorySize(std::istream& stream) {
    size_t size;
    stream >> size;
    return size;
}

std::vector<MemoryManagerQuery> ReadMemoryManagerQueries(
    std::istream& stream) {
    size_t queries_count;
    stream >> queries_count;
    std::vector<MemoryManagerQuery> queries;
    for (size_t ii = 0; ii < queries_count; ++ii) {
        int query;
        stream >> query;
        if (query > 0) {
            AllocationQuery allocation_query;
            allocation_query.allocation_size = query;
            queries.push_back(MemoryManagerQuery(allocation_query));
        } else {
            FreeQuery free_query;
            free_query.allocation_query_index = -query - 1;
            queries.push_back(MemoryManagerQuery(free_query));
        }
    }
    return queries;
}

void OutputMemoryManagerResponses(
    const std::vector<MemoryManagerAllocationResponse>& responses,
    std::ostream& ostream) {
    for (size_t i = 0; i < responses.size(); ++i) {
        if (responses[i].success) {
            ostream << responses[i].position << std::endl;
        } else {
            ostream << -1 << std::endl;
        }
    }
}

/* Закончили определять функции ввода/вывода */

std::vector<MemoryManagerAllocationResponse> RunMemoryManager(
    size_t memory_size,
    const std::vector<MemoryManagerQuery>& queries) {

    MemoryManager manager(memory_size);
    std::vector<MemoryManagerAllocationResponse> responses;
    std::vector<MemorySegmentIterator> queries_iterators;

    for (size_t query_index = 0; query_index < queries.size(); ++query_index) {
        if (const AllocationQuery* allocation_query = queries[query_index].AsAllocationQuery()) {
            MemorySegmentIterator allocate_segment =
                manager.Allocate(queries[query_index].AsAllocationQuery()->allocation_size);
            if (allocate_segment != manager.end()) {
                queries_iterators.push_back(allocate_segment);
                responses.push_back(MakeSuccessfulAllocation(allocate_segment->left));
            } else {
                queries_iterators.push_back(manager.end());
                responses.push_back(MakeFailedAllocation());
            }
        } else {
            if (queries_iterators[queries[query_index].AsFreeQuery()->allocation_query_index] !=
                manager.end()) {
                manager.Free(
                    queries_iterators[queries[query_index].AsFreeQuery()->allocation_query_index]
                );
            }
            queries_iterators.push_back(manager.end());
        }
    }
    return responses;
}

MemoryManagerAllocationResponse MakeSuccessfulAllocation(size_t position) {
    MemoryManagerAllocationResponse response;
    response.position = position;
    response.success = true;
    return response;
}

MemoryManagerAllocationResponse MakeFailedAllocation() {
    MemoryManagerAllocationResponse response;
    response.success = false;
    return response;
}
