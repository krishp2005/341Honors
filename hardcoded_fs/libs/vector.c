/**
 * vector
 * CS 341 - Fall 2024
 */
#include "vector.h"
#include "callbacks.h"
#include <assert.h>
#include <string.h>

struct vector
{
    /* The function callback for the user to define the way they want to copy
     * elements */
    copy_constructor_type copy_constructor;

    /* The function callback for the user to define the way they want to destroy
     * elements */
    destructor_type destructor;

    /* The function callback for the user to define the way they a default
     * element to be constructed */
    default_constructor_type default_constructor;

    /* Void pointer to the beginning of an array of void pointers to arbitrary
     * data. */
    void **array;

    /**
     * The number of elements in the vector.
     * This is the number of actual objects held in the vector,
     * which is not necessarily equal to its capacity.
     */
    size_t size;

    /**
     * The size of the storage space currently allocated for the vector,
     * expressed in terms of elements.
     */
    size_t capacity;
};

/**
 * IMPLEMENTATION DETAILS
 *
 * The following is documented only in the .c file of vector,
 * since it is implementation specfic and does not concern the user:
 *
 * This vector is defined by the struct above.
 * The struct is complete as is and does not need any modifications.
 *
 * The only conditions of automatic reallocation is that
 * they should happen logarithmically compared to the growth of the size of the
 * vector inorder to achieve amortized constant time complexity for appending to
 * the vector.
 *
 * For our implementation automatic reallocation happens when -and only when-
 * adding to the vector makes its new  size surpass its current vector capacity
 * OR when the user calls on vector_reserve().
 * When this happens the new capacity will be whatever power of the
 * 'GROWTH_FACTOR' greater than or equal to the target capacity.
 * In the case when the new size exceeds the current capacity the target
 * capacity is the new size.
 * In the case when the user calls vector_reserve(n) the target capacity is 'n'
 * itself.
 * We have provided get_new_capacity() to help make this less ambigious.
 */

static size_t get_new_capacity(size_t target)
{
    /**
     * This function works according to 'automatic reallocation'.
     * Start at 1 and keep multiplying by the GROWTH_FACTOR untl
     * you have exceeded or met your target capacity.
     */
    size_t new_capacity = 1;
    while (new_capacity < target)
    {
        new_capacity *= GROWTH_FACTOR;
    }
    return new_capacity;
}

vector *vector_grow(vector *this)
{
    assert(this);

    size_t cap = get_new_capacity(this->capacity + 1);
    void **new_arr = realloc(this->array, cap * sizeof(void *));
    assert(new_arr);
    this->array = new_arr;
    this->capacity = cap;

    return this;
}

vector *vector_create(copy_constructor_type copy_constructor, destructor_type destructor,
                      default_constructor_type default_constructor)
{
    vector *v = malloc(sizeof(vector));
    v->copy_constructor = copy_constructor;
    v->destructor = destructor;
    v->default_constructor = default_constructor;

    v->array = malloc(sizeof(void *) * INITIAL_CAPACITY);
    v->size = 0;
    v->capacity = INITIAL_CAPACITY;

    return v;
}

void vector_destroy(vector *this)
{
    assert(this);
    vector_clear(this);
    free(this->array);
    free(this);
}

void **vector_begin(vector *this)
{
    assert(this);
    return this->array;
}

void **vector_end(vector *this)
{
    assert(this);
    return this->array + this->size;
}

size_t vector_size(vector *this)
{
    assert(this);
    return this->size;
}

void vector_resize(vector *this, size_t n)
{
    assert(this);

    // call Drop on the rest of the elements
    for (size_t i = n; i < this->size; ++i)
        this->destructor(this->array[i]);

    // increase capacity if needed
    vector_reserve(this, n);

    // fill the array with the rest of the elements
    for (size_t i = this->size; i < n; ++i)
        this->array[i] = this->default_constructor();

    // set the size :)
    this->size = n;
}

size_t vector_capacity(vector *this)
{
    assert(this);
    return this->capacity;
}

bool vector_empty(vector *this)
{
    assert(this);
    return this->size == 0;
}

void vector_reserve(vector *this, size_t n)
{
    assert(this);
    assert(this->array);
    while (n > this->capacity)
        vector_grow(this);
}

void **vector_at(vector *this, size_t position)
{
    assert(this);
    assert(position < this->size);
    return &this->array[position];
}

void vector_set(vector *this, size_t position, void *element)
{
    assert(this);
    assert(this->array);
    assert(position < this->size);

    // Drop always
    // this assumes that array[position] is initialized
    this->destructor(this->array[position]);

    // create copy of element
    this->array[position] = this->copy_constructor(element);
}

void *vector_get(vector *this, size_t position)
{
    assert(this);
    assert(this->array);
    assert(position < this->size);
    return this->array[position];
}

void **vector_front(vector *this)
{
    assert(this);
    assert(this->array);
    return vector_begin(this);
}

void **vector_back(vector *this)
{
    assert(this);
    assert(!vector_empty(this));
    return this->array + this->size - 1;
}

void vector_push_back(vector *this, void *element)
{
    assert(this);
    vector_reserve(this, this->size + 1);
    this->array[this->size++] = this->copy_constructor(element);
}

void vector_pop_back(vector *this)
{
    assert(this);
    assert(this->array);
    assert(!vector_empty(this));
    this->destructor(*vector_back(this));
    --this->size;
}

void vector_insert(vector *this, size_t position, void *element)
{
    assert(this);
    assert(position <= this->size);

    vector_reserve(this, this->size + 1);
    // this is safe, because we have allocated enough memory for the destination
    // memmove is required, as src and dest will almost always overlap
    // no Drops need to be called :)
    memmove(&this->array[position + 1], &this->array[position], sizeof(void *) * (this->size - position));
    this->array[position] = this->copy_constructor(element);
    ++this->size;
}

void vector_erase(vector *this, size_t position)
{
    assert(this);
    assert(position < vector_size(this));

    this->destructor(vector_get(this, position));
    memmove(&this->array[position], &this->array[position + 1], sizeof(void *) * (this->size - position));
    this->array[this->size--] = NULL;
}

void vector_clear(vector *this)
{
    assert(this);
    assert(this->array);
    vector_resize(this, 0);
}

// The following is code generated
void *string_copy_constructor(void *elem)
{
    if (!elem)
        return NULL;
    return strdup(elem);
}

void string_destructor(void *elem)
{
    free(elem);
}

void *string_default_constructor(void)
{
    return calloc(1, 1);
}

vector *string_vector_create(void)
{
    return vector_create(string_copy_constructor, string_destructor, string_default_constructor);
}
