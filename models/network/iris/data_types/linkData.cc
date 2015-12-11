#include	"linkData.h"
#include "kernel/serialize.h"

using namespace std;
using namespace manifold::iris;


namespace manifold {
namespace kernel {

template<>
size_t
Serialize<LinkData>(LinkData* p,unsigned char* buf )
{
    int pos = 0;

    memcpy(buf+pos,&(p->src), sizeof(uint)); pos+=sizeof(uint);
    memcpy(buf+pos,&(p->vc), sizeof(uint)); pos+=sizeof(uint);
    memcpy(buf+pos,&(p->type), sizeof(int)); pos+=sizeof(int);

    if ( p->type == FLIT)
    {
        memcpy(buf+pos,&(p->f->type), sizeof(int)); pos+=sizeof(int);
#ifdef _DEBUG2
        memcpy(buf+pos,&p->f->flit_id, sizeof(uint)); pos+=sizeof(uint);
#endif
        if ( p->f->type == HEAD )
        {
            const HeadFlit* hf = static_cast<const HeadFlit*>(p->f);
            //pack base class members first
            memcpy(buf+pos,&hf->virtual_channel, sizeof(uint)); pos+=sizeof(uint);
            memcpy(buf+pos,&hf->pkt_length, sizeof(uint)); pos+=sizeof(uint);
            //pack HeadFlit members
            memcpy(buf+pos,&hf->src_id, sizeof(uint)); pos+=sizeof(uint);
            memcpy(buf+pos,&hf->dst_id, sizeof(uint)); pos+=sizeof(uint);
            memcpy(buf+pos,&hf->mclass, sizeof(int)); pos+=sizeof(int);
            memcpy(buf+pos,&hf->enter_network_time, sizeof(uint64_t)); pos+=sizeof(uint64_t);
            memcpy(buf+pos,&hf->data_len, sizeof(uint64_t)); pos+=sizeof(int);
            memcpy(buf+pos,&hf->data, hf->data_len*sizeof(uint8_t));
            pos += hf->data_len * sizeof(uint8_t);
            delete hf; //LinkData doesn't own the flit, so it must be deleted separately.
        }
        else if ( p->f->type == BODY )
        {
            const BodyFlit* bf = static_cast<const BodyFlit*>(p->f);
            //pack base class members first
            memcpy(buf+pos,&bf->virtual_channel, sizeof(uint)); pos+=sizeof(uint);
            memcpy(buf+pos,&bf->pkt_length, sizeof(uint)); pos+=sizeof(uint);
            //pack BodyFlit members
            memcpy(buf+pos,&bf->data, HeadFlit::MAX_DATA_SIZE*sizeof(uint8_t));
            pos += HeadFlit::MAX_DATA_SIZE * sizeof(uint8_t);

            delete bf; //LinkData doesn't own the flit, so it must be deleted separately.
        }
        else if ( p->f->type == TAIL )
        {
            const TailFlit* tf = static_cast<const TailFlit*>(p->f);
            //pack base class members first
            memcpy(buf+pos,&tf->virtual_channel, sizeof(uint)); pos+=sizeof(uint);
            memcpy(buf+pos,&tf->pkt_length, sizeof(uint)); pos+=sizeof(uint);

            delete tf; //LinkData doesn't own the flit, so it must be deleted separately.
        }
        else
        {
            cout << " ERROR Invalid flit type " << p->f->type << endl;
            cout.flush();
            exit(1);
        }
    }

    delete p;
    return pos;
}

template<>
size_t
Get_serialize_size<LinkData>(const LinkData* ld)
{ 
    size_t size = sizeof(int) * 3; //type, vc, src

    if(ld->type == FLIT) {
        size += sizeof(int); //ld->f->type
#ifdef _DEBUG2
        size += sizeof(int); //ld->f->flit_id
#endif
        if ( ld->f->type == HEAD ) {
            size += sizeof(HeadFlit); //sizeof(HeadFlit) may be slightly bigger than the sum of 
            //individual fields, but this is ok.
        }
        else if ( ld->f->type == BODY ) {
            size += sizeof(BodyFlit);
        }
        else if ( ld->f->type == TAIL ) {
            size += sizeof(TailFlit);
        }
        else
            assert(0);
    }
    return size;
}


template<>
LinkData*
Deserialize<LinkData>(unsigned char* data )
{ 
    LinkData* ld = new LinkData();
    int pos=0;
    memcpy(&ld->src,data, sizeof(uint)); pos+=sizeof(uint);
    memcpy(&ld->vc,data+pos, sizeof(uint)); pos+=sizeof(uint);
    memcpy(&ld->type,data+pos, sizeof(int)); pos+=sizeof(int);

    if( ld->type == FLIT)
    {
        int ty;
        memcpy(&ty,data+pos, sizeof(int)); pos+=sizeof(int);

        switch ( ty )
        {
            case HEAD:
                {
                    HeadFlit* hf = new HeadFlit();
#ifdef _DEBUG2
                    memcpy(&hf->flit_id,data+pos, sizeof(uint)); pos+=sizeof(uint);
#endif
                    memcpy(&hf->virtual_channel,data+pos, sizeof(uint)); pos+=sizeof(uint);
                    memcpy(&hf->pkt_length,data+pos, sizeof(uint)); pos+=sizeof(uint);
                    memcpy(&hf->src_id,data+pos, sizeof(uint)); pos+=sizeof(uint);
                    memcpy(&hf->dst_id,data+pos, sizeof(uint)); pos+=sizeof(uint);
                    memcpy(&hf->mclass,data+pos, sizeof(int)); pos+=sizeof(int);
                    memcpy(&hf->enter_network_time,data+pos, sizeof(uint64_t)); pos+=sizeof(uint64_t);
                    memcpy(&hf->data_len, data+pos, sizeof(uint64_t)); pos+=sizeof(int);
                    memcpy(&hf->data, data+pos, (hf->data_len)*sizeof(uint8_t));
                    pos += hf->data_len * sizeof(uint8_t);
                    ld->f=hf;
                    break;
                }
            case BODY:
                {
                    BodyFlit* bf = new BodyFlit();
#ifdef _DEBUG2
                    memcpy(&bf->flit_id,data+pos, sizeof(uint)); pos+=sizeof(uint);
#endif
                    memcpy(&bf->virtual_channel,data+pos, sizeof(uint)); pos+=sizeof(uint);
                    memcpy(&bf->pkt_length,data+pos, sizeof(uint)); pos+=sizeof(uint);
                    memcpy(&bf->data, data+pos, (HeadFlit::MAX_DATA_SIZE)*sizeof(uint8_t));
                    pos += HeadFlit::MAX_DATA_SIZE * sizeof(uint8_t);
                    ld->f=bf;
                    break;
                }
            case TAIL:
                {
                    TailFlit* tf = new TailFlit();
#ifdef _DEBUG2
                    memcpy(&tf->flit_id,data+pos, sizeof(uint)); pos+=sizeof(uint);
#endif
                    memcpy(&tf->virtual_channel,data+pos, sizeof(uint)); pos+=sizeof(uint);
                    memcpy(&tf->pkt_length,data+pos, sizeof(uint)); pos+=sizeof(uint);

                    ld->f=tf;
                    break;
                }
            default:
                {

                    cout	<< " Unk flit type in deser " << ty << endl;
                    exit(1);
                }
        }
    }

    return ld; 
}

}//namespace kernel
}//namespace manifold




namespace manifold {
namespace iris {

LinkData::~LinkData()
{
    //do nothing!
    //In particular, LinkData doesn't own the flit, so do not delete the flit!
}


std::string 
LinkData::toString(void) const
{
    std::stringstream str;
    str 
        << " vc: " <<vc 
        << " src_id: " <<src
        << " type: " << type
        ;
    return str.str();
}

/* 
   LinkData
   LinkData::operator=(const LinkData& p )
   {
   LinkData* ld2 = new LinkData();
   ld2->src = p.src;
   ld2->vc = p.vc;
   ld2->type = p.type;

   if ( p.type == FLIT)
   {
   ld2->f = p.f;
   }

   return *ld2;
   }
 * */

} // namespace iris
} // namespace manifold

