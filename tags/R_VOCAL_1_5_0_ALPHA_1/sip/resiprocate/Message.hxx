#ifndef Message_hxx
#define Message_hxx

#include <util/Data.hxx>
#include <iostream>

namespace Vocal2
{

class Message 
{
   public:
      virtual ~Message(){}
      virtual const Data& getTransactionId() const=0;
      virtual Data brief() const=0;
      virtual std::ostream& encode(std::ostream& strm) const=0;
};

std::ostream& 
operator<<(std::ostream& strm, const Vocal2::Message& msg);


}

#endif