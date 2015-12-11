/** @file serialize.h
 * Contains global template functions for data serialization/deserialization.
 * These template functions have default implementations. User only needs to
 * write specializations when necessary.
 *
 * Since data must be copied into kernel's buffer before sending over MPI. The
 * serialize function would provide the buffer, which essentially is the kernel's
 * buffer to minimize data copying. In other words, data types don't need to
 * allocate their own buffer for serialization. However, this also means before
 * serialization we must know the size of serialized data. The function
 * Get_serialize_size() is for this purpose.
 */

#ifndef MANIFOLD_KERNEL_SERIALIZE_H
#define MANIFOLD_KERNEL_SERIALIZE_H
namespace manifold {
namespace kernel {

/**
 * This version of Get_serialize_size() is used on links where the data are sent
 * by value. The default behavior is to return the size of the data. Data types
 * that contain pointer(s) must specialize this template function.
 */
template<typename T>
size_t Get_serialize_size(const T& data)
{
    return sizeof(data);
}

/**
 * This version of Get_serialize_size() is used on links where the data are sent
 * by pointer. The default behavior is to return the size of the data. Data types
 * that contain pointer(s) must specialize this template function.
 */
template<typename T>
size_t Get_serialize_size(const T* data)
{
    return sizeof(*data);
}

/**
 * This version of Serialize() is used on links where the data are sent
 * by value. The default behavior is to simply do a byte-wise copy of the
 * data into the provided buffer.
 *
 * @arg \c data The data being sent.
 * @arg \c buf  The caller-provided buffer to hold serialized data.
 * @return The size of the serialized data.
 */
template<typename T>
size_t Serialize(const T& data, unsigned char* buf)
{
    T* p = (T*) buf;
    *p = data;
    return sizeof(data);
}

/**
 * This version of Serialize() is used on links where the data are sent
 * by pointer. The default behavior is to simply do a byte-wise copy of the
 * data into the provided buffer. Then the pointer is deleted.
 *
 * @arg \c data The pointer to the data being sent.
 * @arg \c buf  The caller-provided buffer to hold serialized data.
 * @return The size of the serialized data.
 */
template<typename T>
size_t Serialize(T* data, unsigned char* buf)
{
    T* p = (T*) buf;
    *p = *data;
    delete data;
    return sizeof(T);
}

/**
 * This version of Serialize() is used on links where the data are sent
 * by const pointer. This should only occur rarely. The default behavior
 * is to simply do a byte-wise copy of the  data into the provided buffer.
 * Since the pointer is const, it is not deleted.
 *
 * @arg \c data The pointer to the data being sent.
 * @arg \c buf  The caller-provided buffer to hold serialized data.
 * @return The size of the serialized data.
 */
template<typename T>
size_t Serialize(const T* data, unsigned char* buf)
{
    T* p = (T*) buf;
    *p = *data;
    //data not deleted!
    return sizeof(T);
}


/**
 * This version of Deserialize() is used on links where the data are sent
 * by value. The default behavior is to simply cast the buffer back into
 * the data type.
 *
 * @arg \c buf  The buffer used to hold serialized data.
 * @arg \c noname The second parameter is only used to differentiate it
 * from another version.
 * @return The data object.
 */
template<typename T>
T Deserialize(unsigned char* buf, int)
{
    return *((T*)buf);
}

/**
 * This version of Deserialize() is used on links where the data are sent
 * by pointer. The default behavior is to re-create the object.
 *
 * @arg \c buf  The buffer used to hold serialized data.
 */
template<typename T>
T* Deserialize(unsigned char* buf)
{
    T* t = new T();
    *t = *((T*)buf);  //byte-wise copy
    return t;
}


} //namespace kernel
} //namespace manifold
#endif // MANIFOLD_KERNEL_SERIALIZE_H
