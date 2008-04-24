#if !defined(RESIP_SIPMESSAGE_HXX)
#define RESIP_SIPMESSAGE_HXX 

#include <sys/types.h>

#include <list>
#include <vector>
#include <utility>
#include <memory> 

#include "resip/stack/Contents.hxx"
#include "resip/stack/Headers.hxx"
#include "resip/stack/TransactionMessage.hxx"
#include "resip/stack/ParserContainer.hxx"
#include "resip/stack/ParserCategories.hxx"
#include "resip/stack/SecurityAttributes.hxx"
#include "resip/stack/Tuple.hxx"
#include "resip/stack/Uri.hxx"
#include "resip/stack/MessageDecorator.hxx"
#include "rutil/BaseException.hxx"
#include "rutil/Data.hxx"
#include "rutil/Timer.hxx"
#include "rutil/HeapInstanceCounter.hxx"

namespace resip
{

class Contents;
class ExtensionHeader;
class SecurityAttributes;
class Transport;

/**
   @ingroup resip_crit
   @brief Represents a SIP message.

   This is the class that your app will spend the most time working with. This
   is because, in the UA core/Transaction User architecture, the vast majority
   of interaction is carried out through SIP messaging.

   When you get a SipMessage, generally the first thing you want to know is 
   whether it is a request or a response. This is done by calling 
   SipMessage::isRequest() or SipMessage::isResponse().
   
   Next, it is usually important to determine what the SIP method of the message 
   is. This is done by calling SipMessage::method() (this is a convenience 
   function that checks the method of the Request-Line if the message is a 
   request, or the method of the CSeq if a response).
   
   At this point, it may become useful to examine the start-line of the message.

   If the message is a request, you can get the Request-Line (represented by a 
   RequestLine&) by calling
   @code
      RequestLine& rLine = sip.header(h_RequestLine);
   @endcode

   If the message is a response, you can get the Status-Line (represented by a StatusLine&) by calling
   @code
      StatusLine& sLine = sip.header(h_StatusLine);
   @endcode

   From here, examination of the various headers is in order. Each header type
   has an associated h_HeaderName access token that we use to retrieve the 
   value/values of that header. Generally speaking, the access token 
   h_HeaderName is named in a predictable fashion; all non-alphanumeric 
   characters are omitted, the first letter of each word is capitalized, and the 
   name is pluralized if the header is multi-valued. Examples include h_To, 
   h_From, h_CSeq, h_CallId, h_Routes, h_Contacts, h_RecordRoutes, etc.

*/
class SipMessage : public TransactionMessage
{
   public:
      RESIP_HeapCount(SipMessage);
      typedef std::list< std::pair<Data, HeaderFieldValueList*> > UnknownHeaders;

      explicit SipMessage(const Transport* fromWire = 0);
      // .dlb. public, allows pass by value to compile.
      SipMessage(const SipMessage& message);

      // .dlb. sure would be nice to have overloaded return value here..
      virtual Message* clone() const;

      SipMessage& operator=(const SipMessage& rhs);
      
      // returns the transaction id from the branch or if 2543, the computed hash
      virtual const Data& getTransactionId() const;

      const Data& getRFC2543TransactionId() const;
      void setRFC2543TransactionId(const Data& tid);
      
      virtual ~SipMessage();

      static SipMessage* make(const Data& buffer, bool isExternal = false);
      void parseAllHeaders();
      
      static bool checkContentLength;

      class Exception : public BaseException
      {
         public:
            Exception(const Data& msg, const Data& file, const int line)
               : BaseException(msg, file, line) {}

            const char* name() const { return "SipMessage::Exception"; }
      };

      void setFromTU() 
      {
         mIsExternal = false;
      }

      void setFromExternal()
      {
         mIsExternal = true;
      }
      
      bool isExternal() const
      {
         return mIsExternal;
      }

      virtual bool isClientTransaction() const;
      
      virtual std::ostream& encode(std::ostream& str) const;      
      //sipfrags will not output Content Length if there is no body--introduce
      //friendship to hide this?
      virtual std::ostream& encodeSipFrag(std::ostream& str) const;
      std::ostream& encodeEmbedded(std::ostream& str) const;
      
      virtual std::ostream& encodeBrief(std::ostream& str) const;

      bool isRequest() const;
      bool isResponse() const;
      bool isInvalid() const{return mInvalid;}
      
      resip::MethodTypes method() const;
      
      const resip::Data& getReason() const{return mReason;}
      
      const RequestLine& 
      header(const RequestLineType& l) const;

      RequestLine& 
      header(const RequestLineType& l);

      const StatusLine& 
      header(const StatusLineType& l) const;

      StatusLine& 
      header(const StatusLineType& l);

      bool exists(const HeaderBase& headerType) const;
      bool empty(const HeaderBase& headerType) const;
      void remove(const HeaderBase& headerType);

#include "resip/stack/SipMessage.hxx.ixx"

      // unknown header interface
      const StringCategories& header(const ExtensionHeader& symbol) const;
      StringCategories& header(const ExtensionHeader& symbol);
      bool exists(const ExtensionHeader& symbol) const;
      void remove(const ExtensionHeader& symbol);

      // typeless header interface
      const HeaderFieldValueList* getRawHeader(Headers::Type headerType) const;
      void setRawHeader(const HeaderFieldValueList* hfvs, Headers::Type headerType);
      const UnknownHeaders& getRawUnknownHeaders() const {return mUnknownHeaders;}

      Contents* getContents() const;
      // removes the contents from the message
      std::auto_ptr<Contents> releaseContents();

      void setContents(const Contents* contents);
      void setContents(std::auto_ptr<Contents> contents);

      // transport interface
      void setStartLine(const char* start, int len); 

      void setBody(const char* start, UInt32 len); 
      
      // add HeaderFieldValue given enum, header name, pointer start, content length
      void addHeader(Headers::Type header,
                     const char* headerName, int headerLen, 
                     const char* start, int len);

      // Interface used to determine which Transport was used to receive a
      // particular SipMessage. If the SipMessage was not received from the
      // wire, getReceivedTransport() returns 0. Set in constructor
      const Transport* getReceivedTransport() const { return mTransport; }

      // Returns the source tuple that the message was received from
      // only makes sense for messages received from the wire
      void setSource(const Tuple& tuple) { mSource = tuple; }
      const Tuple& getSource() const { return mSource; }
      
      // Used by the stateless interface to specify where to send a request/response
      void setDestination(const Tuple& tuple) { mDestination = tuple; }
      Tuple& getDestination() { return mDestination; }

      void addBuffer(char* buf);

      // returns the encoded buffer which was encoded by
      // TransportSelector::transmit()
      // !!! should only be called by the TransportSelector !!!
      Data& getEncoded();

      // returns the compartment ID which was computed by
      // TransportSelector::transmit()
      // !!! should only be called by the TransportSelector !!!
      Data& getCompartmentId();

      UInt64 getCreatedTimeMicroSec() {return mCreatedTime;}

      // deal with a notion of an "out-of-band" forced target for SIP routing
      void setForceTarget(const Uri& uri);
      void clearForceTarget();
      const Uri& getForceTarget() const;
      bool hasForceTarget() const;

      const Data& getTlsDomain() const { return mTlsDomain; }
      void setTlsDomain(const Data& domain) { mTlsDomain = domain; }

      const std::list<Data>& getTlsPeerNames() const { return mTlsPeerNames; }
      void setTlsPeerNames(const std::list<Data>& tlsPeerNames) { mTlsPeerNames = tlsPeerNames; }

      Data getCanonicalIdentityString() const;
      
      SipMessage& mergeUri(const Uri& source);      

      void setSecurityAttributes(std::auto_ptr<SecurityAttributes>) const;
      const SecurityAttributes* getSecurityAttributes() const { return mSecurityAttributes.get(); }

      void addOutboundDecorator(MessageDecorator *md){mOutboundDecorators.push_back(md);}
      void callOutboundDecorators(const Tuple &src, const Tuple &dest);
      bool mIsDecorated;

      bool mIsBadAck200;

   protected:
      void cleanUp();
   
   private:
      void compute2543TransactionHash() const;

      std::ostream& 
      encode(std::ostream& str, bool isSipFrag) const;      

      void copyFrom(const SipMessage& message);

      HeaderFieldValueList* ensureHeaders(Headers::Type type, bool single);
      HeaderFieldValueList* ensureHeaders(Headers::Type type, bool single) const; // throws if not present

      // indicates this message came from the wire, set by the Transport
      bool mIsExternal;
      
      // raw text corresponding to each typed header (not yet parsed)
      mutable HeaderFieldValueList* mHeaders[Headers::MAX_HEADERS];

      // raw text corresponding to each unknown header
      mutable UnknownHeaders mUnknownHeaders;
  
      // !jf!
      const Transport* mTransport;

      // For messages received from the wire, this indicates where it came
      // from. Can be used to get to the Transport and/or reliable Connection
      Tuple mSource;

      // Used by the TU to specify where a message is to go
      Tuple mDestination;
      
      // Raw buffers coming from the Transport. message manages the memory
      std::vector<char*> mBufferList;

      // special case for the first line of message
      mutable HeaderFieldValueList* mStartLine;

      // raw text for the contents (all of them)
      mutable HeaderFieldValue* mContentsHfv;

      // lazy parser for the contents
      mutable Contents* mContents;

      // cached value of a hash of the transaction id for a message received
      // from a 2543 sip element. as per rfc3261 see 17.2.3
      mutable Data mRFC2543TransactionId;

      // is a request or response
      mutable bool mRequest;
      mutable bool mResponse;

      bool mInvalid;
      resip::Data mReason;
      
      Data mEncoded; // to be retransmitted
      Data mCompartmentId; // for retransmissions
      UInt64 mCreatedTime;

      // used when next element is a strict router OR 
      // client forces next hop OOB
      Uri* mForceTarget;

      // domain associated with this message for tls cert
      Data mTlsDomain;

      // peers domain associate with this message (MTLS)
      std::list<Data> mTlsPeerNames; 

      mutable std::auto_ptr<SecurityAttributes> mSecurityAttributes;

      std::vector<MessageDecorator*> mOutboundDecorators;

      friend class TransportSelector;
};

}

#undef ensureHeaderTypeUseable
#undef ensureSingleHeader
#undef ensureMultiHeader
#undef defineHeader
#undef defineMultiHeader

#endif

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
