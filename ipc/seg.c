#include "seg.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>

typedef void* seg_t;

typedef struct seg_tag {
    uint32_t seg_size     : 31;
    uint32_t is_available : 1;
    uint32_t offset_from_base_free_seg; // -1 - no free seg
} *seg_tag_t;

typedef struct { // stored at the end of the provided memory
    uint32_t offset_from_base_ff;
} aux_state;

static size_t next_power_of_2(size_t a);

// size of payload data
size_t    seg__data_size(seg_t seg);
size_t    seg__seg_size_to_data_size(size_t seg_size);
size_t    seg__data_size_to_seg_size(size_t data_size);
// header tag of segment
seg_tag_t seg__head(seg_t seg);
// tailer tag of segment, head's size must be initialized before calling this
seg_tag_t seg__tail(seg_t seg);
// get size of segment
size_t    seg__size(seg_t seg);
// set size of segment
void      seg__set_size(seg_t seg, size_t segment_size);
// true if segment is available
bool      seg__is_available(seg_t seg);
// next seg in memory, wraps, so care about infinite loops
seg_t     seg__next(memory_slice_t memory, seg_t seg);
// prev seg in memory, wraps, so care about infinite loops
seg_t     seg__prev(memory_slice_t memory, seg_t seg);
seg_t     seg__find_next_free(memory_slice_t memory, seg_t seg);
seg_t     seg__find_prev_free(memory_slice_t memory, seg_t seg);
// next free seg
seg_t     seg__next_free(memory_slice_t memory, seg_t seg);
// prev free seg
seg_t     seg__prev_free(memory_slice_t memory, seg_t seg);
// try to coal with neighbors, seg can be either available or not, returns resulting grown seg
seg_t     seg__coal(memory_slice_t memory, seg_t seg);
// returns the size of coaling without applying coal
size_t    seg__check_coal_size(memory_slice_t memory, seg_t seg);
// removes part of the seg, seg must be not available, returns the seg with the required size [!A] -> [!A][A]
seg_t     seg__detach(memory_slice_t memory, seg_t seg, size_t new_seg_size);
// copy 'size' bytes from the start of src's data to the start of dst's data
void      seg__copy_data(seg_t dst, seg_t src, size_t size);
uint32_t  seg__get_offset(memory_slice_t memory, seg_t seg);

static void* seg__seg_to_data(seg_t seg);
static seg_t seg__data_to_seg(void* data);
static seg_t seg__internal_free(memory_slice_t memory, seg_t seg);

// get the first free pointer
static seg_t seg__get_ff(memory_slice_t memory);
// seg the first free pointer
static void seg__set_ff(memory_slice_t memory, seg_t seg);

typedef struct {
    size_t seg_size;
    bool   available;
    seg_t  prev_free;
    seg_t  next_free;
} seg_state_t;

void seg_state__init(memory_slice_t memory, seg_t seg, size_t seg_size, bool available, seg_t prev_free, seg_t next_free);
seg_state_t seg_state__get(memory_slice_t memory, seg_t seg);
// --== state transitions ==--
void seg_state__available(memory_slice_t memory, seg_t seg, bool value);

// Sequential Fit Policies
/*  First Fit
    Searches the list of free blocks from 'seg' and uses the first block large enough to satisfy the request.
    If the block is larger than necessary, it is split and the remainder is put in the free list.
    Problem: The larger blocks near the beginning of the list tend to be split first, and the remaining fragments
    result in a lot of small free blocks near the beginning of the list.
*/
static seg_t seg__first_fit(memory_slice_t memory, size_t requested_seg_size);
// /*  Next Fit
//     Search from segment X with first fit policy.
//     Policies for X:
//         Roving pointer: Ideally this pointer always points to the largest segment. This is achieved by setting it
//         to the segment immediately followed by the last allocation performed (often, this will be sufficiently
//         big for the next allocation).
// */
// static seg_t seg__next_fit(memory_slice_t memory, size_t requested_seg_size);
// /*  Best Fit
//     Search the entire list to find the smallest, that is large enough to satisfy the request.
//     It may stop if a perfect fit is found earlier.
//     Problem: Depending on how the data structure is implemented, this can be slow.
// */
// static seg_t seg__best_fit(memory_slice_t memory, size_t requested_seg_size);

seg_tag_t seg__head(seg_t seg) {
    assert(seg);

    return (seg_tag_t) seg;
}

seg_tag_t seg__tail(seg_t seg) {
    assert(seg);
    
    return (seg_tag_t) ((uint8_t*) seg + seg__size(seg) - sizeof(struct seg_tag));
}

static void* seg__seg_to_data(seg_t seg) {
    if (!seg) {
        return 0;
    }

    return (uint8_t*) seg + sizeof(struct seg_tag);
}

size_t seg__data_size(seg_t seg) {
    seg_tag_t head = seg__head(seg);
    seg_tag_t tail = seg__tail(seg);
    if (head == tail) {
        return 0;
    }
    else {
        return seg__seg_size_to_data_size(seg__size(seg));
    }
}

size_t seg__seg_size_to_data_size(size_t seg_size) {
    return seg_size - 2 * sizeof(struct seg_tag);
}

size_t seg__data_size_to_seg_size(size_t data_size) {
    return data_size + 2 * sizeof(struct seg_tag);
}

size_t seg__size(seg_t seg) {
    seg_tag_t head = seg__head(seg);
    return head->seg_size;
}

void seg__set_size(seg_t seg, size_t segment_size) {
    seg_tag_t head = seg__head(seg);
    head->seg_size = segment_size;

    // note can't find tail before segment_size is set
    seg_tag_t tail = seg__tail(seg);
    tail->seg_size = segment_size;
}

static seg_t seg__first_fit(memory_slice_t memory, size_t requested_seg_size) {
    seg_t seg = seg__get_ff(memory);
    seg_t seg_start = seg;
    while (seg) {
        assert(seg__is_available(seg));

        size_t seg_size = seg__data_size(seg);
        if (seg_size >= requested_seg_size) {
            return seg;
        }

        seg = seg__next_free(memory, seg);

        if (seg == seg_start) {
            break ;
        }
    }

    return 0;
}

// static seg_t seg__next_fit(memory_slice_t memory, size_t requested_seg_size) {
//     // todo: implement
//     return seg__first_fit(memory, requested_seg_size);
// }

// static seg_t seg__best_fit(memory_slice_t memory, size_t requested_seg_size) {
//     seg_t best_seg = 0;
//     seg_t cur_seg = seg__get_ff(memory);
//     while (cur_seg) {
//         assert(seg__is_available(cur_seg));

//         size_t seg_size = seg__data_size(cur_seg);
//         if (seg_size >= requested_seg_size) {
//             if (best_seg == 0) {
//                 best_seg = cur_seg;
//             } else if (seg_size < seg__data_size(best_seg)) {
//                 best_seg = cur_seg;
//             }
//         }

//         cur_seg = seg__next_free(cur_seg);
//     }

//     return best_seg;
// }

seg_t seg__next(memory_slice_t memory, seg_t seg) {
    assert(seg);
    assert(memory.memory);

    seg_t next_seg = (uint8_t*) seg + seg__size(seg);

    const uint8_t* end_of_memory = (uint8_t*) memory.memory + memory.size - sizeof(aux_state);
    if ((uint8_t*) next_seg == end_of_memory) {
        next_seg = (seg_t) memory.memory;
    }
    assert(!(end_of_memory < (uint8_t*) next_seg));

    return next_seg;
}

seg_t seg__prev(memory_slice_t memory, seg_t seg) {
    assert(seg);
    assert(memory.memory);
    assert(!((uint8_t*) seg < (uint8_t*) memory.memory));

    if (seg == memory.memory) {
        uint8_t* end_of_memory = (uint8_t*) memory.memory + memory.size - sizeof(aux_state);
        seg_tag_t last_seg_tail = (seg_tag_t)(end_of_memory - sizeof(struct seg_tag));
        seg_t last_seg = end_of_memory - last_seg_tail->seg_size;
        return last_seg;
    }
    
    seg_tag_t prev_seg_tail = (seg_tag_t) ((uint8_t*) seg - sizeof(struct seg_tag));
    assert(0 < prev_seg_tail->seg_size);

    return (uint8_t*) seg - prev_seg_tail->seg_size;
}

seg_t seg__find_next_free(memory_slice_t memory, seg_t seg) {
    seg_t seg_start = seg;
    seg = seg__next(memory, seg);    
    while (seg && seg != seg_start) {
        if (seg__is_available(seg)) {
            return seg;
        }
        seg = seg__next(memory, seg);
    }

    return 0;
}

seg_t seg__find_prev_free(memory_slice_t memory, seg_t seg) {
    seg = seg__prev(memory, seg);
    while (seg) {
        if (seg__is_available(seg)) {
            return seg;
        }
        seg = seg__prev(memory, seg);
    }

    return 0;
}

seg_t seg__next_free(memory_slice_t memory, seg_t seg) {
    uint32_t offset_from_base_free_seg = seg__tail(seg)->offset_from_base_free_seg;
    if (offset_from_base_free_seg == (uint32_t) -1) {
        return 0;
    }

    seg_t result = (seg_t)((uint8_t*) memory.memory + offset_from_base_free_seg);
    if (result == seg) {
        return 0;
    }
    assert(seg__is_available(result));

    return result;
}

seg_t seg__prev_free(memory_slice_t memory, seg_t seg) {
    uint32_t offset_from_base_free_seg = seg__head(seg)->offset_from_base_free_seg;
    if (offset_from_base_free_seg == (uint32_t) -1) {
        return 0;
    }

    seg_t result = (seg_t)((uint8_t*) memory.memory + offset_from_base_free_seg);
    if (result == seg) {
        return 0;
    }
    assert(seg__is_available(result));

    return result;
}

bool seg__is_available(seg_t seg) {
    return seg__head(seg)->is_available;
}

seg_t seg__coal(memory_slice_t memory, seg_t seg) {
    assert(seg);
    
    seg_t prev = seg__prev(memory, seg);
    seg_t next = seg__next(memory, seg);

    bool is_prev_available = prev && seg__is_available(prev);
    bool is_next_available = next && seg__is_available(next);

    if (!is_prev_available && !is_next_available) {
    } else if (is_prev_available && !is_next_available) {
        /*
            [ A ][ A/!A ]
                    ^
        */

        size_t new_size = seg__size(seg) + seg__size(prev);

        if (seg__is_available(seg)) {
            seg_t next_free = seg__next_free(memory, seg);
            if (next_free) {
                seg_state__available(memory, seg, false);

                seg__tail(seg)->seg_size = new_size;
                seg__head(prev)->seg_size = new_size;
                
                seg__tail(prev)->offset_from_base_free_seg = seg__get_offset(memory, next_free);
                seg__tail(prev)->is_available = true;
            }
        } else {
            size_t seg_data_size = seg__data_size(seg);
            void*  seg_dst       = seg__seg_to_data(prev);
            void*  seg_src       = seg__seg_to_data(seg);

            seg_state__available(memory, prev, false);

            seg__tail(seg)->seg_size = new_size;
            seg__head(prev)->seg_size = new_size;

            assert(seg_data_size < seg__data_size(prev));
            memmove(seg_dst, seg_src, seg_data_size);
        }
    } else if (!is_prev_available && is_next_available) {
        /*
            [ A/!A ][ A ]
               ^
        */

        size_t new_size = seg__size(seg) + seg__size(next);

        if (seg__is_available(seg)) {  
            seg_t next_free = seg__next_free(memory, next);
            if (next_free) {
                seg_state__available(memory, next, false);

                seg__tail(next)->seg_size = new_size;
                seg__head(seg)->seg_size = new_size;
                
                seg__tail(seg)->offset_from_base_free_seg = seg__get_offset(memory, next_free);
                seg__tail(seg)->is_available = true;
            }

        } else {
            seg_state__available(memory, next, false);

            seg__tail(next)->seg_size = new_size;
            seg__head(seg)->seg_size = new_size;
        }
    } else {
        /*
            [ A ][ A/!A ][ A ]
                    ^
        */

        size_t new_size = seg__size(seg) + seg__size(prev) + seg__size(next);

        if (seg__is_available(seg)) {
            seg_t next_free = seg__next_free(memory, next);
            if (next_free) {
                seg_state__available(memory, seg, false);
                seg_state__available(memory, next, false);

                seg__tail(next)->seg_size = new_size;
                seg__head(prev)->seg_size = new_size;

                seg__tail(prev)->offset_from_base_free_seg = seg__get_offset(memory, next_free);
                seg__tail(prev)->is_available = true;
            }
        } else {
            size_t seg_data_size = seg__data_size(seg);
            void*  seg_dst       = seg__seg_to_data(prev);
            void*  seg_src       = seg__seg_to_data(seg);

            seg_state__available(memory, prev, false);
            seg_state__available(memory, next, false);

            seg__tail(next)->seg_size = new_size;
            seg__head(prev)->seg_size = new_size;

            assert(seg_data_size < seg__data_size(prev));
            memmove(seg_dst, seg_src, seg_data_size);
        }
    }

    if (is_prev_available) {
        seg = prev;
    }
    
    return seg;
}

size_t seg__check_coal_size(memory_slice_t memory, seg_t seg) {
    assert(seg);
    
    seg_t prev = seg__prev(memory, seg);
    seg_t next = seg__next(memory, seg);

    bool is_prev_available = prev && seg__is_available(prev);
    bool is_next_available = next && seg__is_available(next);

    if (!is_prev_available && !is_next_available) {
        return seg__size(seg);
    } else if (is_prev_available && !is_next_available) {
        return seg__size(seg) + seg__size(prev);
    } else if (!is_prev_available && is_next_available) {
        return seg__size(seg) + seg__size(next);
    } else {
        return seg__size(seg) + seg__size(prev) + seg__size(next);
    }
}

seg_t seg__detach(memory_slice_t memory, seg_t seg, size_t new_seg_size) {
    // store state of seg
    seg_state_t state = seg_state__get(memory, seg);
    assert(!state.available);

    if (new_seg_size == state.seg_size) {
        return seg;
    }

    assert(new_seg_size < state.seg_size);

    size_t detachment_size = state.seg_size - new_seg_size;
    seg_t  detachment = (seg_t) ((uint8_t*) seg + new_seg_size);

    // do not support 0 size segments
    if (detachment_size <= seg__data_size_to_seg_size(0)) {
        return seg;
    }

    // initialize the two new segments
    seg_state__init(memory, detachment, detachment_size, false, 0, 0);
    seg_state__init(memory, seg, new_seg_size, state.available, state.prev_free, state.next_free);

    assert(seg__tail(seg)->offset_from_base_free_seg == (uint32_t) -1);
    assert(seg__head(seg)->offset_from_base_free_seg == (uint32_t) -1);

    // free the detachment
    (void) seg__internal_free(memory, detachment);

    return seg;
}

void seg_state__init(memory_slice_t memory, seg_t seg, size_t seg_size, bool available, seg_t prev_free, seg_t next_free) {
    assert(seg);

    seg__head(seg)->seg_size = seg_size;
    seg__tail(seg)->seg_size = seg_size;
    seg__head(seg)->offset_from_base_free_seg = seg__get_offset(memory, prev_free);
    seg__tail(seg)->offset_from_base_free_seg = seg__get_offset(memory, next_free);
    seg__head(seg)->is_available = available;
    seg__tail(seg)->is_available = available;
}

seg_state_t seg_state__get(memory_slice_t memory, seg_t seg) {
    seg_state_t result = {
        .seg_size = seg__size(seg),
        .available = seg__is_available(seg),
        .prev_free = seg__prev_free(memory, seg),
        .next_free = seg__next_free(memory, seg)
    };

    return result;
}

void seg_state__available(memory_slice_t memory, seg_t seg, bool value) {
    assert(seg);
    assert(seg__is_available(seg) != value);

    if (seg__is_available(seg)) {
        seg__head(seg)->is_available = value;
        seg__tail(seg)->is_available = value;

        seg_t ff = seg__get_ff(memory);
        assert(ff);

        seg_t prev_free = seg__prev_free(memory, seg);
        seg_t next_free = seg__next_free(memory, seg);

        seg__head(seg)->offset_from_base_free_seg = (uint32_t) -1;
        seg__tail(seg)->offset_from_base_free_seg = (uint32_t) -1;

        if (prev_free) {
            seg__tail(prev_free)->offset_from_base_free_seg = seg__get_offset(memory, next_free);
        }
        if (next_free) {
            seg__head(next_free)->offset_from_base_free_seg = seg__get_offset(memory, prev_free);
        }

        if (ff == seg) {
            if (prev_free) {
                seg__set_ff(memory, prev_free);
            } else if (next_free) {
                seg__set_ff(memory, next_free);
            } else {
                seg__set_ff(memory, 0);
            }
        } else {
            assert(prev_free);
        }
    } else {
        seg__head(seg)->is_available = value;
        seg__tail(seg)->is_available = value;

        seg_t prev_free = seg__find_prev_free(memory, seg);
        seg_t next_free = seg__find_next_free(memory, seg);

        seg__head(seg)->offset_from_base_free_seg = seg__get_offset(memory, prev_free);
        seg__tail(seg)->offset_from_base_free_seg = seg__get_offset(memory, next_free);

        if (prev_free) {
            seg__tail(prev_free)->offset_from_base_free_seg = seg__get_offset(memory, seg);
        }
        if (next_free) {
            seg__head(next_free)->offset_from_base_free_seg = seg__get_offset(memory, seg);
        }

        seg_t ff = seg__get_ff(memory);
        if (!ff) {
            seg__set_ff(memory, seg);
        } else if ((uint8_t*) seg < (uint8_t*) ff) {
            assert(next_free);
            assert(ff == next_free);
            seg__set_ff(memory, seg);
        }
    }
}

bool seg__init(memory_slice_t memory) {
    size_t mem_size = memory.size;
    if (mem_size < sizeof(aux_state)) {
        return false;
    }

    seg_t seg = (seg_t) memory.memory;
    seg__set_ff(memory, seg);

    seg_state__init(memory, seg, mem_size - sizeof(aux_state), true, 0, 0);

    return true;
}

bool seg__print(memory_slice_t memory, void* ptr) {
    seg_t seg = seg__data_to_seg(ptr);
    seg_tag_t tag = seg__head(seg);
    seg_t next = seg__next(memory, seg);
    seg_t expected = (seg_t) ((uint8_t*) seg + tag->seg_size);
    if ((seg_t) ((uint8_t*) memory.memory + memory.size - sizeof(aux_state)) == expected) {
        expected = (seg_t) memory.memory;
    }
    printf(
        "[<- %12p, %12p, %s, %8u, %12p%s, %12p ->]\n",
        seg__prev_free(memory, seg),
        seg,
        tag->is_available ? "free" : "used",
        tag->seg_size,
        next ? expected : 0,
        next ? (((uint8_t*) expected == (uint8_t*) next) ? "" : " ??") : "",
        seg__next_free(memory, seg)
    );

    return true;
}

void seg__print_aux(memory_slice_t memory) {
    printf("--== Aux start      ==--\n");
    printf("[ ff: %12p ]\n", (seg_t) seg__get_ff(memory));
    printf("--== Aux end        ==--\n");
}

void seg__for_each(memory_slice_t memory, bool (*fn)(memory_slice_t memory, void* ptr)) {
    seg_t seg = (seg_t) memory.memory;
    seg_t seg_start = seg;

    while (seg) {
        seg_t next = seg__next(memory, seg);
        if (!fn(memory, seg__seg_to_data(seg))) {
            break ;
        }
        seg = next;
        if (seg == seg_start) {
            break ;
        }
    }

}

void* seg__get_next_taken(memory_slice_t memory, void* ptr) {
    seg_t seg_result = !ptr ? (seg_t) memory.memory : seg__data_to_seg(ptr);
    seg_t seg_start = seg_result;
    do {
        if (!seg__is_available(seg_result)) {
            return seg__seg_to_data(seg_result);
        }
        seg_result = seg__next(memory, seg_result);
        if (!seg_result || seg_result == seg_start) {
            return 0;
        }
    } while (1);
}

static size_t next_power_of_2(size_t a) {
    if (a == 0) {
        return 1;
    }

    --a;
    a |= a >> 1;
    a |= a >> 2;
    a |= a >> 4;
    a |= a >> 8;
    a |= a >> 16;
    a |= a >> 32;
    
    return a + 1;
}

void* seg__malloc(memory_slice_t memory, size_t size) {
    // look for available with one of the sequential fit policy

    size = seg__seg_size_to_data_size(next_power_of_2(seg__data_size_to_seg_size(size)));
    seg_t seg = seg__first_fit(memory, size);
    if (!seg) {
        return 0;
    }

    // make seg unavailable
    seg_state__available(memory, seg, false);
    // detach the unnecessary segment
    seg = seg__detach(memory, seg, seg__data_size_to_seg_size(size));

    return seg__seg_to_data(seg);
}

void* seg__calloc(memory_slice_t memory, size_t size) {
    void* result = seg__malloc(memory, size);
    if (!result) {
        return 0;
    }

    memset(result, 0, seg__data_size(seg__data_to_seg(result)));
    return result;
}

void* seg__realloc(memory_slice_t memory, void* ptr, size_t new_size) {
    assert(new_size != 0);

    seg_t seg = 0;

    if (!ptr) {
        return seg__malloc(memory, new_size);
    }

    seg = seg__data_to_seg(ptr);
    assert(!seg__is_available(seg));

    size_t old_data_size = seg__data_size(seg);
    if (old_data_size < new_size) {
        // coal if it would satisfy the request
        size_t coal_data_size = seg__seg_size_to_data_size(seg__check_coal_size(memory, seg));

        size_t required_seg_size = seg__data_size_to_seg_size(new_size);
        size_t coal_seg_size = seg__data_size_to_seg_size(coal_data_size);

        bool is_coal_big_enough = coal_data_size >= new_size;
        bool is_coal_bigger_than_smallest_seg = is_coal_big_enough && (coal_seg_size - required_seg_size > seg__data_size_to_seg_size(0));

        if (is_coal_bigger_than_smallest_seg) {
            seg = seg__coal(memory, seg);

            // detach the unnecessary segment
            seg = seg__detach(memory, seg, seg__data_size_to_seg_size(new_size));
            return seg__seg_to_data(seg);
        }

        // todo: copy and free before looking for sequential fit

        // malloc new seg
        void* new_data = seg__malloc(memory, new_size);
        if (!new_data) {
            return 0;
        }

        // copy ptr to new seg
        seg__copy_data(seg__data_to_seg(new_data), seg, seg__data_size(seg));
        // free seg
        seg__internal_free(memory, seg);

        return new_data;
    } else if (new_size < old_data_size) {
        // detach the unnecessary segment
        seg = seg__detach(memory, seg, seg__data_size_to_seg_size(new_size));
        return seg__seg_to_data(seg);
    } else {
        return seg__seg_to_data(seg);
    }

    assert(false);
}

void* seg__recalloc(memory_slice_t memory, void* ptr, size_t new_size) {
    assert(new_size != 0);

    seg_t seg = 0;

    if (!ptr) {
        return seg__malloc(memory, new_size);
    }

    seg = seg__data_to_seg(ptr);
    assert(!seg__is_available(seg));

    size_t old_data_size = seg__data_size(seg);
    if (old_data_size < new_size) {
        // coal if it would satisfy the request
        size_t coal_data_size = seg__seg_size_to_data_size(seg__check_coal_size(memory, seg));

        size_t required_seg_size = seg__data_size_to_seg_size(new_size);
        size_t coal_seg_size = seg__data_size_to_seg_size(coal_data_size);

        bool is_coal_big_enough = coal_data_size >= new_size;
        bool is_coal_bigger_than_smallest_seg = is_coal_big_enough && (coal_seg_size - required_seg_size > seg__data_size_to_seg_size(0));

        if (is_coal_bigger_than_smallest_seg) {
            seg = seg__coal(memory, seg);

            // detach the unnecessary segment
            seg = seg__detach(memory, seg, seg__data_size_to_seg_size(new_size));
            return seg__seg_to_data(seg);
        }

        // todo: copy and free before looking for sequential fit

        // malloc new seg
        void* new_data = seg__malloc(memory, new_size);
        if (!new_data) {
            return 0;
        }

        // copy ptr to new seg
        seg__copy_data(seg__data_to_seg(new_data), seg, seg__data_size(seg));
        // zero out rest of the memory
        memset((uint8_t*) new_data + old_data_size, 0, new_size - old_data_size);

        // free seg
        seg__internal_free(memory, seg);

        return new_data;
    } else if (new_size < old_data_size) {
        // detach the unnecessary segment
        seg = seg__detach(memory, seg, seg__data_size_to_seg_size(new_size));
        return seg__seg_to_data(seg);
    } else {
        return seg__seg_to_data(seg);
    }

    assert(false);
}

void seg__free(memory_slice_t memory, void* ptr) {
    if (!ptr) {
        int dbg = 0;
        ++dbg;
    }
    assert(ptr);
    seg__internal_free(memory, seg__data_to_seg(ptr));
}

static seg_t seg__internal_free(memory_slice_t memory, seg_t seg) {
    if (seg__is_available(seg)) {
        int dbg = 0;
        ++dbg;
    }
    assert(!seg__is_available(seg));
    seg_state__available(memory, seg, true);

    // libc__printf("------- BEFORE COAL START -------\n");
    // seg__for_each(memory, &seg__print);
    // libc__printf("------- BEFORE COAL END   -------\n");

    seg_t coaled_seg = seg__coal(memory, seg);

    // libc__printf("------- AFTER COAL START -------\n");
    // seg__for_each(memory, &seg__print);
    // libc__printf("------- AFTER COAL END   -------\n");

    return coaled_seg;
}

static seg_t seg__data_to_seg(void* data) {
    if (!data) {
        return 0;
    }
    
    return (seg_t) ((uint8_t*) data - sizeof(struct seg_tag));
}

void seg__copy_data(seg_t dst, seg_t src, size_t size) {
    size_t src_size = seg__head(src)->seg_size;
    size_t dst_size = seg__head(dst)->seg_size;
    assert(src_size <= dst_size);

    memmove(seg__seg_to_data(dst), seg__seg_to_data(src), size);
}

uint32_t seg__get_offset(memory_slice_t memory, seg_t seg) {
    if (!seg) {
        return (uint32_t) -1;
    }

    assert((uint8_t*) memory.memory <= (uint8_t*) seg);
    return (uint8_t*) seg - (uint8_t*) memory.memory;
}

static seg_t seg__get_ff(memory_slice_t memory) {
    void* data = memory.memory;
    size_t data_size = memory.size;
    aux_state* aux = (aux_state*) ((uint8_t*) data + data_size - sizeof(aux_state));
    if (aux->offset_from_base_ff == -1) {
        return 0;
    }

    return (seg_t) ((uint8_t*) memory.memory + aux->offset_from_base_ff);
}

static void seg__set_ff(memory_slice_t memory, seg_t seg) {
    void* data = memory.memory;
    size_t data_size = memory.size;
    aux_state* aux = (aux_state*) ((uint8_t*) data + data_size - sizeof(aux_state));
    aux->offset_from_base_ff = seg__get_offset(memory, seg);
}
