/** @file component-decl.h
 *  Common definitions for Manifold
 *
 *
 * This file contains the declarations (not the implementation)
 * of the Component class.  The actual implementation is in 
 * component.h. We split them apart since we have circular dependencies
 * with component requiring manifold, and manifold requiring component.
 */
 
 //George F. Riley, (and others) Georgia Tech, Fall 2010
 

#ifndef MANIFOLD_KERNEL_COMPONENT_DECL_H__
#define MANIFOLD_KERNEL_COMPONENT_DECL_H__

#include "common-defs.h"
#include "link-decl.h"
#include <string>
#include <map>

namespace manifold {
namespace kernel {

class Component;

//! \class ComponentLpMapping component-decl.h
//!  Stores the logical process id for each
//!  component in the system with associated name. For distributed simulations, 
//!  all components are created on all LP's; only those "mapped" to that LP
//!  are actually created, other use null pointers. 
class ComponentLpMapping 
{
public:

  //! @arg \c c Component ptr
  //!  @arg \c l Logic process of the component
  ComponentLpMapping(Component* c, LpId_t l)
    : component(c), lp(l) {}

  //! Pointer to the component being mapped, nil if it is on a remote Lp.
  Component* component;
  
  //! Lp id for the component this mapping refers to.
  LpId_t     lp;
};


//! @class BorderPort component-decl.h
//! @brief Object of this class is associated with a border port and is used to
//! predict time-stamp of future output events.
class BorderPort {
public:
    BorderPort(int srcp, int srclp, int dstlp, Clock* clk) :
        m_SRCPORT(srcp), m_SRCLP(srclp), m_DSTLP(dstlp), m_clk(clk) {}

    //! Inform scheduler tick of next output.
    void update_output_tick(Ticks_t);
    Clock* get_clk() { return m_clk; }
private:
    const int m_SRCPORT;
    const int m_SRCLP;
    const int m_DSTLP;
    Clock* m_clk;
};


//! @class LinkTypeMismatchException component-decl.h
//!
//! A LinkTypeMismatchException is thrown when trying to set up a link
//! to a component input where a link with a different data type already
//! exists.
class LinkTypeMismatchException
{
};


//! @class CompName component-decl.h
//! This class is only used in the Component::Creat() functions. It's constructor is
//! explicit so it is type-different from char* or string. The purpose is to avoid
//! template instantiation ambiguity that occurs when a component constructor's last
//! parameter happens to be char* or string.
class CompName {
public:
    explicit CompName(const char* nm) : m_name(nm) {}
    std::string& get_name() { return m_name; }

private:
    std::string m_name;
};


//! @class Component component-decl.h
//! The base class for all models.
class Component 
{
public:
  
  Component();
  virtual ~Component();

  void set_clock(Clock& c) { m_clk = &c; }
  Clock* get_clock() { return m_clk; }
  
  int getComponentId() const { return myId; }    
  void setComponentId(CompId_t newId) { myId = newId; }

  std::string  getComponentName() const { return myName; } 
  void setComponentName(std::string& newName) { myName = newName; }
  
  //! Creates a complex link that sends arbitrary type T
  //! @arg \c outIndex The index(port) number of the Link
  template <typename T> Link<T>* AddOutputLink(int outIndex) throw (LinkTypeMismatchException);
  

  //! Add an input for the component.
  //! @arg \c inputIndex The index(port) number of the input.
  //! @arg \c handler Event handler for the input. It takes 2 parameters, and integer and T2 the type of the input data. 
  //! @arg \c obj Event handling object, almost always the component itself.
  //! @arg \c c Associated clock, if any.
  //! @arg \c isTimed A boolean indicating if the link delay is specified in Time_t or Ticks_t.
  //! @arg \c isHalf A boolean indicating if the link delay unit is half-tick.
  template<typename T, typename T2>
  void AddInput(int inputIndex, void (T::*handler)(int, T2), T* obj, Clock* c, bool isTimed=false, bool isHalf=false);

  //! Used to send data on an output link of any type
  //!  @arg The output index(port) number to send the data on. 
  //!  @arg The actual data being sent over the Link.
  template <typename T>
    void Send(int, T);
    
  template <typename T>
    void SendTick(int, T, Ticks_t);

  template <typename T>
    void SendTime(int, T, Time_t);
    
  //! This is called by the scheduler to handler an incoming message.
  template <typename T>
  void Recv_remote(int inputIndex, Ticks_t sendTick, Ticks_t recvTick,
                   Time_t sendTime, Time_t recvTime, T data);

  void Recv_remote(int inputIndex, Ticks_t sendTick, Ticks_t recvTick,
                   Time_t sendTime, Time_t recvTime, unsigned char* data, int len);


  void add_border_port(int srcPort, int dstLP, Clock* clk);

  Clock* getInLinkClock(int inputIndex) { //get the clock for an input link
      return inLinks[inputIndex]->getClock();
  }


  //#################
  // Static functions
  //#################
  
  //! Return true if component is local to this LP
  //! @arg Component id
  static bool IsLocal(CompId_t);

  //! Return true if component is local to this LP
  //! @arg \c name component name
  static bool IsLocal(const std::string& name);

  /** Variation on Create, 0 arguments for the constructor.
   *  The first argument is always type LpId_t, and indicates which LP
   * is desired; subsequent parameters are passed to the constructor.
   * @arg Component id
   * @arg Component name (optional)
   */
  template <typename T>
    static CompId_t Create(LpId_t, CompName name = CompName("None"));

  /** Variation on Create, 1 arugment for the constructor.
   *  The first argument is always type LpId_t, and indicates which LP
   * is desired; subsequent parameters are passed to the constructor.
   * @arg Lp id
   * @arg Any type argument
   * @arg Component name (optional)
   */    
  template <typename T, typename T1> 
    static CompId_t Create(LpId_t, T1&, CompName name = CompName("None"));
  template <typename T, typename T1> 
    static CompId_t Create(LpId_t, const T1&, CompName name = CompName("None"));

  /** Variation on Create, 2 arugments for the constructor.
   *  The first argument is always type LpId_t, and indicates which LP
   * is desired; subsequent parameters are passed to the constructor.
   * @arg Lp id
   * @arg Any type argument
   * @arg Any type argument
   * @arg Component name (optional)
   */        
  template <typename T, typename T1, typename T2> 
    static CompId_t Create(LpId_t, const T1&, const T2&, CompName name = CompName("None"));
  template <typename T, typename T1, typename T2> 
    static CompId_t Create(LpId_t, T1&, T2&, CompName name = CompName("None"));

  /** Variation on Create, 3 arugments for the constructor.
   *  The first argument is always type LpId_t, and indicates which LP
   * is desired; subsequent parameters are passed to the constructor.
   * @arg Lp id
   * @arg Any type argument
   * @arg Any type argument
   * @arg Any type argument   
   * @arg Component name (optional)
   */        
  template <typename T, typename T1, typename T2, typename T3> 
    static CompId_t Create(LpId_t, const T1&, const T2&, const T3&, CompName name = CompName("None"));
  template <typename T, typename T1, typename T2, typename T3> 
    static CompId_t Create(LpId_t, T1&, T2&, T3&, CompName name = CompName("None"));

  /** Variation on Create, 4 arugments for the constructor.
   *  The first argument is always type LpId_t, and indicates which LP
   *  is desired; subsequent parameters are passed to the constructor.
   * @arg Lp id
   * @arg Any type argument
   * @arg Any type argument
   * @arg Any type argument
   * @arg Any type argument   
   * @arg Component name (optional)
   */        
  template <typename T, typename T1, typename T2, typename T3, typename T4> 
    static CompId_t Create(LpId_t, const T1&, const T2&, const T3&, const T4&, CompName name = CompName("None"));
  template <typename T, typename T1, typename T2, typename T3, typename T4> 
    static CompId_t Create(LpId_t, T1&, T2&, T3&, T4&, CompName name = CompName("None"));

  template <typename T, typename T1, typename T2, typename T3, typename T4, typename T5> 
    static CompId_t Create(LpId_t, const T1&, const T2&, const T3&, const T4&, const T5&, CompName name = CompName("None"));

  template <typename T, typename T1, typename T2, typename T3, typename T4, typename T5> 
    static CompId_t Create(LpId_t, T1&, T2&, T3&, T4&, T5&, CompName name = CompName("None"));

  //! Static function to get the original component pointer by component id
  //!  @arg CompId_t
  template <typename T>
    static T* GetComponent(CompId_t);
   
  //! Static function to get the original component pointer by component name
  //!  @arg \c name component name
  template <typename T>
    static T* GetComponent(const std::string& name);
 
  //! Static function to get the component, returning base class pointer
  //!  by component id. Note it's not a template.
  //!  @arg Component id
  static Component* GetComponent(CompId_t);
  
  //! Static function to get the component, returning base class pointer
  //!  by component name. Note it's not a template.
  //!  @arg \c name component name
  static Component* GetComponent(const std::string& name);
 
  //! Static function to get the component LP by component id
  //!  @arg Component id
  static LpId_t GetComponentLP(CompId_t);
  
  //! Static function to get the component LP by component name
  //!  @arg Component name
  static LpId_t GetComponentLP(const std::string& name);


protected:
  
  std::vector<LinkInputBase*> inLinks;

  std::vector<BorderPort*> border_ports;

  virtual void remote_input_notify(Ticks_t when, void*, int port) {}

  
  //! Vector of output link ptrs for this component
  std::vector<LinkBase*> outLinks;

  Clock* m_clk; //the clock with which the component is registered.


private:
  
  void setComponentName(CompName& newName) { myName = newName.get_name(); }


  //! Component id for this component.
  CompId_t myId;
 
  //! Component name for this component.
  std::string myName; 

  //! Next available component id
  static CompId_t nextId;
  
  //! vector of all componentlpmappings for all components in the system
  static std::vector<ComponentLpMapping> AllComponents;

  //! maps component name to id
  static std::map<std::string, CompId_t> AllNames;
};

} //namespace kernel
} //namespace manifold


#endif
