/* auto-generated by genmsg_cpp from /u/ethand/rosPostCommonMsgs/ros-pkg/sandbox/compressed_video_streaming/msg/binaryblob.msg.  Do not edit! */
#ifndef COMPRESSED_VIDEO_STREAMING_BINARYBLOB_H
#define COMPRESSED_VIDEO_STREAMING_BINARYBLOB_H

#include <string>
#include <vector>
#include "ros/message.h"
#include "ros/time.h"

namespace compressed_video_streaming
{

//! \htmlinclude binaryblob.msg.html

class binaryblob : public ros::Message
{
public:
  typedef boost::shared_ptr<binaryblob> Ptr;
  typedef boost::shared_ptr<binaryblob const> ConstPtr;

  typedef std::vector<uint8_t> _blob_type;

  std::vector<uint8_t> blob;

  binaryblob() : ros::Message()
  {
  }
  binaryblob(const binaryblob &copy) : ros::Message()
  {
    (void)copy;
    blob = copy.blob;
  }
  binaryblob &operator =(const binaryblob &copy)
  {
    if (this == &copy)
      return *this;
    blob.clear();
    blob = copy.blob;
    return *this;
  }
  virtual ~binaryblob() 
  {
    blob.clear();
  }
  inline static std::string __s_getDataType() { return std::string("compressed_video_streaming/binaryblob"); }
  inline static std::string __s_getMD5Sum() { return std::string("686a5a6faa4b2c7d2070ef2a260d09e7"); }
  inline static std::string __s_getMessageDefinition()
  {
    return std::string(
    "uint8[] blob\n"
    "\n"
    );
  }
  inline virtual const std::string __getDataType() const { return __s_getDataType(); }
  inline virtual const std::string __getMD5Sum() const { return __s_getMD5Sum(); }
  inline virtual const std::string __getMessageDefinition() const { return __s_getMessageDefinition(); }
  void set_blob_size(uint32_t __ros_new_size)
  {
    this->blob.resize(__ros_new_size);
  }
  inline uint32_t get_blob_size() const { return blob.size(); }
  inline void get_blob_vec (std::vector<uint8_t> &__ros_vec) const
  {
    __ros_vec = this->blob;
  }
  inline void set_blob_vec(const std::vector<uint8_t> &__ros_vec)
  {
    this->blob = __ros_vec;
  }
  inline uint32_t serializationLength() const
  {
    unsigned __l = 0;
    __l += 4 + (blob.size() ? blob.size() * 1 : 0); // blob
    return __l;
  }
  virtual uint8_t *serialize(uint8_t *write_ptr, uint32_t seq) const
  {
    uint32_t __blob_size = blob.size();
    SROS_SERIALIZE_PRIMITIVE(write_ptr, __blob_size);
    memcpy(write_ptr, &blob[0], sizeof(uint8_t) * __blob_size);
    write_ptr += sizeof(uint8_t) * __blob_size;
    return write_ptr;
  }
  virtual uint8_t *deserialize(uint8_t *read_ptr)
  {
    uint32_t __blob_size;
    SROS_DESERIALIZE_PRIMITIVE(read_ptr, __blob_size);
    set_blob_size(__blob_size);
    memcpy(&blob[0], read_ptr, sizeof(uint8_t) * __blob_size);
    read_ptr += sizeof(uint8_t) * __blob_size;
    return read_ptr;
  }
};

typedef boost::shared_ptr<binaryblob> binaryblobPtr;
typedef boost::shared_ptr<binaryblob const> binaryblobConstPtr;


}

#endif