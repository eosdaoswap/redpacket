#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <utils.hpp>
#define max(a,b) (a>b?a:b)

using namespace eosio;

class [[eosio::contract("redpacket")]] redpacket : public eosio::contract
{   

public:

    using contract::contract;

   [[eosio::on_notify("*::transfer")]]
   void on_token(const name& from, const name& to, const asset& quantity, const std::string&  memo);

   [[eosio::action]]
   void openredsae(const name& owner,const uint64_t& redid,const std::string& secret,const std::string& verify);   

   [[eosio::action]]
   void closeredsae(const name& owner,const uint64_t& redid);

   [[eosio::action]]
   void mgrclear(const name& owner,const uint64_t& redid);

   [[eosio::action]]
   void geturl(const name& owner,const uint64_t& redid);

   [[eosio::action]]
   void mgrtoken(const name& controller,const uint64_t& tokenid,const symbol& sym,const name& contract,const uint8_t& mgrstates);

   [[eosio::action]]
   void mgrtoken2(const name& controller,const uint64_t& tokenid,const uint8_t& states);

   [[eosio::action]]
   void mgrprice(const name& controller,const uint64_t& tokenid,const double& price);

   [[eosio::action]]
   void mgrnft(const name& controller,const name& owner,const uint64_t& nftid,const std::string& title,const std::string& nfturl,const uint8_t& states,const uint8_t& mgrstates);

   [[eosio::action]]
   void mgrmkt(const name& controller,const uint64_t& mktid, const uint64_t& swapid,const std::string& swapname,const uint64_t& pairid,const std::string& pairname,const uint8_t& states,const uint8_t& mgrstates);

   [[eosio::action]]
   void mgrdiycond(const name& controller,const name& owner,uint64_t customcondid, const uint8_t& customtype,const std::string& title, const asset& quantity, const std::string& customize,const uint8_t& mgrstates);

   [[eosio::action]]
   void usrdiycond(const name& owner,uint64_t customcondid,const uint8_t& customtype,const std::string& title, const asset& quantity, const std::string& customize,const uint8_t& mgrstates);

   [[eosio::action]]
   void deldiycond(const name& owner,uint64_t customcondid);

   [[eosio::action]]
   void mgrcustomize(const name& controller,const name& owner,const uint64_t& customcondid,const asset& quantity,const std::string& customize);

   [[eosio::action]]
   void usrcustomize(const name& owner,const uint64_t& customcondid,const asset& quantity,const std::string& customize);

   [[eosio::action]]
   void mgrclosered(const name& controller,const name& owner,const uint64_t& redid);

   [[eosio::action]]
   void mgrdeletered(const name& controller,const name& owner,const uint64_t& redid);

   [[eosio::action]]
   void mgrwltgroup(const name& controller,const name& owner,const uint64_t& groupid,const std::string& title,const std::string& content,const uint8_t& states,const uint8_t& mgrstates);

   [[eosio::action]]
   void userwltgroup(const name& owner,const uint64_t& groupid,const std::string& title,const std::string& content,const uint8_t& states,const uint8_t& mgrstates);

   [[eosio::action]]
   void userwltcls(const name& owner,const uint64_t& groupid);

   [[eosio::action]]
   void mgrwltdtl(const name& controller,const name& owner,const uint64_t& groupid,const name& user,const uint8_t& mgrstates);

   [[eosio::action]]
   void userwltdtl(const name& owner,const uint64_t& groupid,const name& user,const uint8_t& mgrstates);

   [[eosio::action]]
   void mgrtest(const name& owner,const uint64_t& redid);

   [[eosio::action]]
   void test2(const name& owner,const uint64_t& redid);

   [[eosio::action]]
   void test3(const name& owner,const uint64_t& redid);

   [[eosio::action]]
   void test4(const name& owner,const symbol& sym);


private:

    /*
    * socpe:_self
    */
    struct [[eosio::table]] global_var
    {
        name        key;
        uint64_t    val;

        uint64_t primary_key() const { return key.value; }
    };
    typedef multi_index<name("globals"), global_var> globals_index;

   /*
   * socpe:_self
   * condition: 伪码|红包类型|代币ID|代币合约|封面ID|红包开始时间|红包结束时间|抢红包方式（1随机，2固定）|白名单群组ID|黑名单群组ID|自定义条件ID|祝福语ID|
   */   
   struct [[eosio::table]] redsaelist {
      uint64_t    redid; 
      name        owner;                              
      asset       quantity1; //总额    
      asset       quantity2; //已抢金额          
      uint16_t    num1; //红包总个数
      uint16_t    num2;  //已抢个数
 std::string      condition;//红包条件
      uint8_t     redsaetype;//1红包广场()；2个人红包(personal)；3新人红包
      bool        signcond;//门槛标志:false，无；true，有    
      bool        states; //false，关闭；true，正常                                   
      uint64_t    acttime;             

      uint64_t    primary_key() const { return redid; }    
      uint64_t    by_owner() const { return owner.value; }        
      uint64_t    by_redsaetype() const { return redsaetype; }                               
      uint64_t    by_states() const { return states; }
      uint64_t    by_acttime() const { return acttime; } 
      uint64_t    by_signcond() const { return signcond; }               
   };
    typedef multi_index<name("redsaelist"), redsaelist,    
         indexed_by<name("owner"), const_mem_fun<redsaelist, uint64_t, &redsaelist::by_owner>>,     
         indexed_by<name("redsaetype"), const_mem_fun<redsaelist, uint64_t, &redsaelist::by_redsaetype>>,       
         indexed_by<name("states"), const_mem_fun<redsaelist, uint64_t, &redsaelist::by_states>>,    
         indexed_by<name("acttime"), const_mem_fun<redsaelist, uint64_t, &redsaelist::by_acttime>>,
         indexed_by<name("signcond"), const_mem_fun<redsaelist, uint64_t, &redsaelist::by_signcond>>                      
      > redsaelist_index;

   /*
   * socpe:_self
   * 
   */   
   struct [[eosio::table]] redsaetiming {
      uint64_t    redid;       
      asset       quantity1; //红包总额         
      asset       quantity2; //红包已抢金额   
      asset       quantity3; //每日红包金额         
      uint16_t    num1; //红包总次数 
      uint16_t    num2; //已抢次数 
      uint16_t    num3; //每日红包个数 
      uint64_t    begintime;//红包开始日期
      uint64_t    endtime;  //红包结束日期         
 std::string      condition;//红包条件  
 	   uint8_t     redsaetype;//红包类型   //1红包广场()；2个人红包(personal)；3新人红包
      bool        signcond;//门槛标志:false，无；true，有    
      bool        states; //false，关闭；true，正常                                   
      uint64_t    acttime;      

      uint64_t    primary_key() const { return redid; }    
      uint64_t     by_redsaetype() const { return redsaetype; }         
      uint64_t    by_states() const { return states; }
      uint64_t    by_acttime() const { return acttime; }               
   };
   typedef multi_index<name("redsaetiming"), redsaetiming,  
         indexed_by<name("redsaetype"), const_mem_fun<redsaetiming, uint64_t, &redsaetiming::by_redsaetype>>,              
         indexed_by<name("states"), const_mem_fun<redsaetiming, uint64_t, &redsaetiming::by_states>>,    
         indexed_by<name("acttime"), const_mem_fun<redsaetiming, uint64_t, &redsaetiming::by_acttime>>                   
      > redsaetiming_index;

   /*
   * socpe:红包ID
   */   
   struct [[eosio::table]] redsaedtl {
      name        user;    
      asset       quantity;               
      uint64_t    acttime;

      uint64_t    primary_key() const { return user.value; }
   };
   typedef eosio::multi_index<name("redsaedtl"), redsaedtl > redsaedtl_index;   


   /*
   * socpe:token
   */   
   struct [[eosio::table("orders")]] newdtl {
        name                owner;

        uint64_t primary_key() const { return owner.value; }
    };
   typedef eosio::multi_index< "newdtl"_n, newdtl> newdtl_index;

   /*
   *socpe:owner
   *每个用户的记录数不超过100条
   */   
   struct [[eosio::table]] usergrab {
      uint64_t    id;
      uint64_t    redid;    
      asset       quantity;               
      uint64_t    acttime;

      uint64_t    primary_key() const { return id; }
   };
   typedef eosio::multi_index<name("usergrab"), usergrab > usergrab_index;   


   /*
   * socpe: 1 - 202203 - 10000 
   *  1/2 发/抢 202203 年月 10001 代币ID
   */   
   struct [[eosio::table]] redsaetotal {
      name        owner;    
      uint64_t    num;        
      asset       quantity;
      asset       money;      
      uint64_t    acttime;  
      
      uint64_t    primary_key() const { return owner.value; }
   };
   typedef eosio::multi_index<name("redsaetotal"), redsaetotal > redsaetotal_index;

  /*
   *socpe:owner
   */   
   struct [[eosio::table]] wltgroup {
      uint64_t       groupid;    
      std::string    title;
      std::string    content;
      uint64_t       acttime; 
      uint8_t        states;            

      uint64_t    primary_key() const { return groupid; }
   };
    typedef multi_index<name("wltgroup"), wltgroup > wltgroup_index;

   /*
   *socpe:groupid
   */   
   struct [[eosio::table]] wltdtl {
      name           user;                       
      uint64_t       acttime;     

      uint64_t    primary_key() const { return user.value; }
   };
    typedef multi_index<name("wltdtl"), wltdtl > wltdtl_index;

   /*
   *socpe:owner
   */   
   struct [[eosio::table]] redsaenft {
      uint64_t       nftid;    
      std::string    title;   
      std::string    nfturl;
      uint8_t        states;                                
      uint64_t       acttime;     

      uint64_t    primary_key() const { return nftid; }
   };
    typedef multi_index<name("redsaenft"), redsaenft > redsaenft_index;

   /*
   *socpe:_self
   */   
   struct [[eosio::table]] redsaetoken {
      uint64_t       tokenid;    
      name           contract;   
      symbol         sym; 
      double         price;                        
      uint8_t        states;     

      uint64_t    primary_key() const { return tokenid; }
   };
    typedef multi_index<name("redsaetoken"), redsaetoken > redsaetoken_index;

   /*
   *socpe:_self
   */   
   struct [[eosio::table]] redsaemkt {
      uint64_t    mktid;  
      uint64_t    swapid;  
      std::string swapname;   
      uint64_t    pairid; 
      std::string pairname; 
      uint64_t    acttime;
      uint8_t     states;    

      uint64_t    primary_key() const { return mktid; }
   };
   typedef eosio::multi_index<name("redsaemkt"), redsaemkt > redsaemkt_index;  

   /*
   *socpe:owner
   */   
   struct [[eosio::table]] customcond {
      uint64_t    customcondid;
      uint8_t     customtype;          
      std::string title;  
      asset       quantity;
      std::string customize;                
      uint8_t     states;          
      uint64_t    acttime;       

      uint64_t    primary_key() const { return customcondid; }
      uint64_t    by_customtype() const { return customtype; }  

   };
   typedef multi_index<name("customcond"), customcond,
         indexed_by<name("customtype"), const_mem_fun<customcond, uint64_t, &customcond::by_customtype>>      
      > customcond_index;
      
   /*
   *socpe:_self
   */  
   struct [[eosio::table]] logs {
        uint64_t id;    
        name owner;  
        std::string opevent;
        uint64_t acttime;                        
        uint64_t primary_key() const { return id; }
    };
   typedef multi_index<"logs"_n, logs> logs_index;   

   uint64_t _redenvelope(const name& from, const asset& quantity, const name& contract,std::vector<std::string> strs,uint64_t redid,const bool& isfee);
   uint64_t _redsaetiming(const name& from, const asset& quantity, const name& contract,std::vector<std::string> strs);
   void _update_total(const name& owner, const name& player, const uint64_t& saletype,const asset& quantity,const asset& money, const uint64_t& ntime,const uint64_t& tokenid);
   asset _get_money(const uint64_t& tokenid,const name& contract,const asset& quantity);
   std::string _get_nft_url(const uint64_t& nftid,const name& owner);
   bool _get_wlt_group(const uint64_t& wltgroupid,const name& owner);
   bool _get_customcond(const uint64_t& customcondid,const name& owner);
   bool _mgrnft(const name& player,const name& owner,const uint64_t& nftid,const std::string& title,const std::string& nfturl,const uint8_t& states,const uint8_t& mgrstates);
   bool _customcond(const name& player,const name& owner,uint64_t customcondid, const uint8_t& customtype,const std::string& title, const asset& quantity, const std::string& customize,const uint8_t& mgrstates);  
   bool _customize(const name& owner,const uint64_t& customcondid,const asset& quantity,const std::string& customize);
   void _wltgroup(const name& player,const name& owner,const uint64_t& groupid,const std::string& title,const std::string& content,const uint8_t& states,const uint8_t& mgrstates);
   void _wltdtl(const name& player,const name& owner,const uint64_t& groupid,const name& user,const uint8_t& mgrstates);

   void _closeredsae(const bool& isauth,const name& player,const name& owner,const uint64_t& redid);
   void _deleteredsae(const bool& isauth,const name& player,const name& owner,const uint64_t& redid);

   name _get_contract(const uint64_t& tokenid);
   bool _get_member(const uint64_t& memberid,const name& owner);
   std::string _get_secretpwd(uint64_t random1,uint64_t random2,uint64_t random3,uint64_t random4,uint64_t random5,std::string secretkey);
   uint64_t _get_id(const name& socpename,const uint64_t& minid);
   bool _manage(const name &account);
   void _close(const name& owner);
   std::string _error_msg(const uint64_t& code,const std::string& msg);  
   void _log(const name& scope, const name& owner, const std::string& opevent);
 
};
