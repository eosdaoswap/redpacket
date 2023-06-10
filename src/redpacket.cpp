#include <redpacket.hpp>
#include <math.h>
#include <ctime>
#include <chrono>
#include <stdio.h>

using namespace std;
using namespace eosio;


void redpacket::on_token(const name& from, const name& to, const asset& quantity, const std::string&  memo)
{
   if(to != get_self())
   {
     return;
   }

   check( from != to ,_error_msg(100201,"Can't transfer to myself !!!"));
   check( quantity.amount > 0 ,_error_msg(100202,"The quantity cannot be zero !!!"));

   std::vector<std::string> strs;
   if( memo.size() <= 0 )
   {
      return;
   }   

   strs = utils::split(memo, "-");
   if(strs.size() < 1)
   {
      return;
   }

   name contract = get_first_receiver(); 
   if(strs[0] == "redenvelope")
   {
      _redenvelope(from,quantity,contract,strs,0,true);
   }
   else if (strs[0] == "redsaetiming")
   {
      _redsaetiming(from,quantity,contract,strs);
   }
}


uint64_t redpacket::_redenvelope(const name& from, const asset& quantity, const name& contract,std::vector<std::string> strs,uint64_t redid,const bool& isfee)
{
   name player = get_self(); 
   uint64_t ntime = current_time_point().sec_since_epoch();
   bool signcond = false;//false表示没有门槛，true表示有门槛
   std::string verify = ""; 
   std::string condition;   
   //asset quantity1 = quantity;
   asset quantity2 = quantity;
   //1:redenvelope-红包数量-代币ID
   //2:redenvelope-红包数量-代币ID-红包金额是否随机-红包类型-封面NFTID-红包开始时间-红包结束时间-白名单群组ID-黑名单群组ID-自定义条件ID-祝福语ID-推荐者
   uint8_t mode = 1; 
   if(strs.size() == 3)
   {
      mode = 1;
   }
   else if (strs.size() == 13)
   {
      mode = 2;
   }
   else
   {
      check(false,_error_msg(200101,"The parameters of the red envelope are wrong! "));
   }

   uint16_t num ; //红包数量 1 -9999
   uint64_t tokenid ;  //代币ID，从下拉框里面选择 

   uint8_t random = 1;  //红包金额是否随机,随机1，固定2  
   uint8_t  redsaetype = 1;  //红包类型  1红包广场()；2个人红包(personal)；3新人红包
   uint64_t nftid = 0 ;    //封面NFTID，从下拉框里面选择  
   uint64_t btime = 0 ;    //红包开始时间  
   uint64_t etime = 0 ;    //红包结束时间  
   uint64_t wltgroupid = 0 ;    //白名单群组ID 
   uint64_t bltgroupid = 0 ;    //黑名单群组ID    
   uint64_t customcondid = 0 ;    //自定义条件ID，从下拉框里面选择   
   uint64_t predid = 0 ;    //定时任务ID   
   name recommender = from ;    //推荐者,    
   asset money =  asset(0, symbol("USDT",4));  

   num = utils::toInt(strs[1]); //红包数量 1 -9999
   tokenid = utils::toInt(strs[2]);  //代币ID，从下拉框里面选择  
  
   if(num < 0 || num > 9999)
   {
      check(false,_error_msg(200311,"exceed the limit 9999 !")); 
   }


   if(mode >= 2)
   {
      random = utils::toInt(strs[3]);   //随机1，固定2  
      redsaetype = utils::toInt(strs[4]);    //红包类型   1红包广场()；2个人红包(personal)；   
      nftid = utils::toInt(strs[5]);    //封面NFTID，从下拉框里面选择 
      btime = utils::toInt(strs[6]);    //红包开始时间  
      etime = utils::toInt(strs[7]);    //红包结束时间  
      wltgroupid = utils::toInt(strs[8]);    //白名单群组ID 
      bltgroupid = utils::toInt(strs[9]);    //黑名单群组ID    
      customcondid = utils::toInt(strs[10]);    //自定义条件ID，从下拉框里面选择   
      predid = utils::toInt(strs[11]);    //定时任务ID   
      recommender = name(strs[12]);    //推荐者,         
   } 


   double price = 0;
   if(tokenid > 0)
   {
      redsaetoken_index _redsaetoken(get_self(), get_self().value);
      auto redsaetoken_itr = _redsaetoken.find(tokenid);
      check(redsaetoken_itr != _redsaetoken.end(),_error_msg(200501,"Token parameters are not obtained  !("+std::to_string(tokenid)+")"));     
      check(redsaetoken_itr->contract == contract,_error_msg(200511,"Incorrect token contract "));
      price = redsaetoken_itr->price;
      money.amount = quantity.amount * price;

      if(redsaetype == 3)
      {
         if(redsaetoken_itr->states!=2)
         {
            check(false,_error_msg(100000,"Please contact the administrator!"));
         }
      }
   }
   else if(tokenid <= 0)
   {
      check(false,_error_msg(200512,"Wrong token parameters  !!!"));     
   }

   check(money.amount >= num * Money_min, _error_msg(200321,"The amount of red envelope cannot be less than 1 usdt"));


   if(isfee)
   {
      quantity2.amount = quantity.amount * Redpacket_tax / 1000;   
      
   }
   else
   {
      quantity2.amount = 0;
   }

   if(random!=1 && random!=2)
   {
      random = 1;
   }
   if(redsaetype <1 || redsaetype > 3)
   {
      redsaetype = 1;
   }   

   _get_nft_url(nftid,from);
   if(btime > 0 || etime > 0)
   {
      if(etime > 0)
      {
         check(ntime<etime,_error_msg(200601,"The deadline cannot be less than the current time !"));
         if(btime > 0)
         {
            check((etime-btime)>3600,_error_msg(200602,"Start time and end time must be greater than 1 hour  !"));
         }
      }
   }
   else
   {
      btime = 0;
      etime = 0;
   }

   signcond = _get_wlt_group(wltgroupid,from);
   bltgroupid = 0;
   if(_get_customcond(customcondid,from))
   {
     signcond = true;
   }   

   check(is_account(recommender),_error_msg(100301,"Wrong account")); 


   if(random == 1 && redsaetype == 1 && btime == 0 && etime == 0 && wltgroupid ==0 && bltgroupid == 0 && customcondid ==0 && predid == 0 && recommender == from)
   {
      mode = 1;
   }

   if(redid == 0)
   {
      redid =  _get_id(name("redenvelope"),1);
      redid = redid * 10000 +  ntime % 10000;
   }  
   //verify = _get_secretpwd(from.value,contract.value,redid,num,ntime,".pwd");

 
   //0|代币ID|代币合约|封面ID|定时任务ID|红包开始时间|红包结束时间|白名单群组ID|黑名单群组ID|自定义条件ID|红包类型|抢红包方式
   //0|代币ID|代币合约
   
   if(mode >= 1)
   {
      condition = "0|"+std::to_string(tokenid)+"|"+contract.to_string();
   }
   if(mode >=2)
   {
     condition +=  "|"+std::to_string(nftid)+"|"+std::to_string(predid)+"|"+std::to_string(btime)+"|"+std::to_string(etime)+"|"+std::to_string(wltgroupid)+"|"+std::to_string(bltgroupid);
     condition +=  "|"+std::to_string(customcondid)+"|"+std::to_string(redsaetype)+"|"+std::to_string(random);
   }

   redsaelist_index _redsaelist(get_self(), get_self().value);
   auto redsaelist_itr = _redsaelist.find(redid);
   check(redsaelist_itr == _redsaelist.end(),_error_msg(200301,"The red envelope does not exist or is closed")); 
   if(redsaelist_itr == _redsaelist.end())
   {
   	_redsaelist.emplace(player, [&](auto& p) {
         p.redid = redid;
         p.owner = from;
         p.quantity1 = quantity;
         p.quantity2 = quantity2;
         p.num1 = num;
         p.num2 = 0;   
         p.condition = condition; 
         p.redsaetype = redsaetype;  
         p.signcond = signcond;
         p.states = true;          
         p.acttime = ntime;       
		});  
   }

   _update_total(from,get_self(),1,quantity,money,ntime,tokenid);
   if(isfee)
   {
      utils::inline_transfer(contract,get_self(),REDPACKET_FEE,quantity2,"");  
   }
   return redid;        
}

uint64_t redpacket::_redsaetiming(const name& from, const asset& quantity, const name& contract,std::vector<std::string> strs)
{
   name player = get_self(); 
   uint64_t ntime = current_time_point().sec_since_epoch();
   bool signcond = false;//false表示没有门槛，true表示有门槛
   std::string verify = ""; 
   std::string condition;   
   //asset quantity1 = quantity;//红包总额
   asset quantity2 = quantity;//红包已抢金额
   quantity2.amount = 0;
   asset quantity3 = quantity;//每日红包金额

   //3:redsaetiming-红包数量-代币ID-红包金额是否随机-红包类型-封面NFTID-红包开始时间-红包结束时间-白名单群组ID-黑名单群组ID-自定义条件ID-祝福语ID-推荐者-时区-红包周期开始时间-红包周期结束时间-每日金额-循环模式

   uint16_t num1 ;//红包总次数
   uint16_t num3 ; //每日红包个数 1 -9999
   uint64_t tokenid ;  //代币ID，从下拉框里面选择 

   uint8_t random = 1;  //红包金额是否随机,随机1，固定2  
   uint8_t  redsaetype = 1;  //红包类型   1红包广场()；2个人红包(personal)；   
   uint64_t nftid = 0 ;    //封面NFTID，从下拉框里面选择  
   uint64_t btime = 0 ;    //红包开始时间  
   uint64_t etime = 0 ;    //红包结束时间  
   uint64_t wltgroupid = 0 ;    //白名单群组ID 
   uint64_t bltgroupid = 0 ;    //黑名单群组ID    
   uint64_t customcondid = 0 ;    //自定义条件ID，从下拉框里面选择   
   uint64_t reserveid = 0 ;    //   预留ID
   name recommender = from ;    //推荐者,      
   uint8_t  timezone = 8;  //时区 
   uint64_t begintime = 0 ;    //红包周期开始时间  
   uint64_t endtime = 0 ;    //红包周期结束时间 
   uint64_t dmoeny ;    //每日金额   
   uint64_t looptype = 1 ;    //循环模式,1日循环 0无
   asset money =  asset(0, symbol("USDT",4));  


   num3 = utils::toInt(strs[1]); //红包数量 1 -9999
   tokenid = utils::toInt(strs[2]);  //代币ID，从下拉框里面选择  
   if(num3 < 0 || num3 > 9999)
   {
      check(false,_error_msg(200311,"exceed the limit 9999 !")); 
   }
   symbol sym;
   double price = 0;
   if(tokenid > 0)
   {
      redsaetoken_index _redsaetoken(get_self(), get_self().value);
      auto redsaetoken_itr = _redsaetoken.find(tokenid);
      check(redsaetoken_itr != _redsaetoken.end(),_error_msg(200501,"Token parameters are not obtained  !("+std::to_string(tokenid)+")"));     
      check(redsaetoken_itr->contract == contract,_error_msg(200511,"Incorrect token contract "));
      sym = redsaetoken_itr->sym;
      price = redsaetoken_itr->price;
      money.amount = quantity.amount * price;
   }
   else if(tokenid <= 0)
   {
      check(false,_error_msg(200512,"Wrong token parameters  !!!"));     
   }

  //人均红包金额不能低于0.1 usdt
  // check(money.amount >= num * Money_min, _error_msg(200321,"The amount of red envelope cannot be less than 1 usdt"));

 //3:redsaetiming-红包数量-代币ID-红包金额是否随机-红包类型-封面NFTID-红包开始时间-红包结束时间-白名单群组ID-黑名单群组ID-自定义条件ID-祝福语ID-推荐者-时区-红包周期开始时间-红包周期结束时间-每日金额-循环模式
   random = utils::toInt(strs[3]);   //红包金额是否随机,随机1，固定2  
   redsaetype = utils::toInt(strs[4]);    //红包类型   1红包广场()；2个人红包(personal)；   
   nftid = utils::toInt(strs[5]);    //封面NFTID，从下拉框里面选择 
   btime = utils::toInt(strs[6]);    //红包开始时间  
   etime = utils::toInt(strs[7]);    //红包结束时间  
   wltgroupid = utils::toInt(strs[8]);    //白名单群组ID 
   bltgroupid = utils::toInt(strs[9]);    //黑名单群组ID    
   customcondid = utils::toInt(strs[10]);    //自定义条件ID，从下拉框里面选择   
   reserveid = utils::toInt(strs[11]);    //  预留ID 
   recommender = name(strs[12]);    //推荐者,    
   
   timezone =  utils::toInt(strs[13]);  //时区      
   begintime = utils::toInt(strs[14]);    //红包周期开始时间  
   endtime = utils::toInt(strs[15]);    //红包周期结束时间 
   dmoeny = utils::toInt(strs[16]);  //每日金额  
   looptype = utils::toInt(strs[17]);    //循环模式,1日循环     
   quantity3.amount = dmoeny * pow(10,sym.precision()); 

   if(random!=1 && random!=2)
   {
      random = 1;
   }
   if(redsaetype!=1 && redsaetype!=2)
   {
      redsaetype = 1;
   }   
   _get_nft_url(nftid,from);
   if(btime > 0 || etime > 0)
   {
      if(etime > 0)
      {
         check(ntime<etime,_error_msg(200601,"The deadline cannot be less than the current time !"));
         if(btime > 0)
         {
            check((etime-btime)>3600,_error_msg(200602,"Start time and end time must be greater than 1 hour  !"));
         }
      }
   }
   else
   {
      btime = 0;
      etime = 0;
   }

   signcond = _get_wlt_group(wltgroupid,from);
   bltgroupid = 0;
   if(_get_customcond(customcondid,from))
   {
     signcond = true;
   }    
   check(is_account(recommender),_error_msg(100301,"Wrong account")); 
   check(timezone <= 24,_error_msg(200610,"Incorrect time zone"));  
   check(endtime>begintime,_error_msg(200606,"The start time should be less than the end time")); 

   uint64_t tmp1 = (begintime + timezone * 3600) / 86400;
   uint64_t tmp2 = (endtime + timezone * 3600) / 86400;
   num1 = tmp2 - tmp1 + 1;

   looptype = 1;
   uint64_t redid =  _get_id(name("redenvelope"),1);
   redid = redid * 10000;
   verify = _get_secretpwd(from.value,contract.value,redid,num1,ntime,".pwd");

   //伪码|代币ID|代币合约|封面ID|预留ID|红包开始时间|红包结束时间|白名单群组ID|黑名单群组ID|自定义条件ID|红包类型|抢红包方式|时区|循环模式
   
   condition = "0|"+std::to_string(tokenid)+"|"+contract.to_string();
   condition +=  "|"+std::to_string(nftid)+"|"+std::to_string(reserveid)+"|"+std::to_string(btime)+"|"+std::to_string(etime)+"|"+std::to_string(wltgroupid)+"|"+std::to_string(bltgroupid);
   condition +=  "|"+std::to_string(customcondid)+"|"+std::to_string(redsaetype)+"|"+std::to_string(random)+"|"+std::to_string(timezone)+"|"+std::to_string(looptype);

   redsaetiming_index _redsaetiming(get_self(), get_self().value);
   auto redsaetiming_itr = _redsaetiming.find(redid);
   check(redsaetiming_itr == _redsaetiming.end(),_error_msg(200901,"Timing record does not exist")); 
   if(redsaetiming_itr == _redsaetiming.end())
   {
      _redsaetiming.emplace(player, [&](auto& p) {
         p.redid = redid;
         p.quantity1 = quantity;
         p.quantity2 = quantity2;
         p.quantity3 = quantity3;
         p.num1 = num1;
         p.num2 = 0;
         p.num3 = num3;
         p.begintime = begintime;  
         p.endtime = endtime;
         p.condition = condition;
         p.redsaetype = redsaetype;
         p.signcond = signcond;        
         p.states = true;          
         p.acttime = ntime;       
      });  
   }

   return redid;   
}

void redpacket::openredsae(const name& owner,const uint64_t& redid,const std::string& secret,const std::string& verify)
{
   require_auth(owner);  

   check(redid > 0,_error_msg(200102,"Bad red envelope ID"));     
   check(verify.size() == 8 ,_error_msg(201001,"Wrong verify parameters  01!!!"));    
   std::string verify1 = _get_secretpwd(owner.value,redid,0,0,0,Secret_client);
   check(verify == verify1 ,_error_msg(201002,"Wrong verify parameters  02!!!"));     

   name player = owner;
   name sender;
   auto ntime = current_time_point().sec_since_epoch();
   uint64_t acttime = 0;   
   bool states = true;

   asset quantity1;
   asset quantity2;
   asset quantity3;   
   uint64_t num1;
   uint64_t num2;
   uint64_t num3;
   std::string condition;
   bool isOK = true;


   redsaedtl_index _redsaedtl(get_self(), redid);
   auto redsaedtl_itr = _redsaedtl.find(owner.value);
   check(redsaedtl_itr == _redsaedtl.end(), _error_msg(200351,"You can't grab red envelopes again!" ));       

   redsaelist_index _redsaelist(get_self(), get_self().value);
   auto redsaelist_itr = _redsaelist.find(redid);
  	check(redsaelist_itr != _redsaelist.end(), _error_msg(200301,"The red envelope does not exist or is closed !!" ));    
   check(redsaelist_itr->states,_error_msg(200303,"The red envelope campaign has ended ,id:" + std::to_string(redid) + ";"));   
   check(redsaelist_itr->num1 != redsaelist_itr->num2, _error_msg(200315,"The number of red envelope records is incorrect"));    

   quantity1 = redsaelist_itr->quantity1;
   quantity2 = redsaelist_itr->quantity2;
   quantity3 = quantity1 - quantity2;
   num1 = redsaelist_itr->num1;
   num2 = redsaelist_itr->num2;    
   num3 = num1 - num2;   
   acttime = redsaelist_itr->acttime;  
   sender =  redsaelist_itr->owner;   
   condition =  redsaelist_itr->condition;

   //伪码|代币ID|代币合约|封面ID|祝福语ID|红包开始时间|红包结束时间|白名单群组ID|黑名单群组ID|自定义条件ID|红包类型|抢红包方式
   //伪码|代币ID|代币合约
  
   std::vector<std::string> strs1;
   strs1 = utils::split(condition, "|");
   if(strs1.size() != 12 && strs1.size() != 3)
   {
      check(false,_error_msg(200811,"The format of the red envelope condition is incorrect"));
   }

   uint64_t tokenid;    //红包开始时间  
   name     contract; //代币合约
   uint64_t btime = 0;    //红包开始时间  
   uint64_t etime = 0;    //红包结束时间  
   uint64_t wltgroupid = 0;    //白名单群组ID 
   uint64_t bltgroupid = 0;    //黑名单群组ID    
   uint64_t customcondid = 0;    //自定义条件ID，从下拉框里面选择  
   uint8_t  redsaetype = 1;    //红包类型   1红包广场()；2个人红包(personal)；  
   uint8_t random = 1;   //随机1，固定2     

   tokenid = utils::toInt(strs1[1]);    //红包开始时间 
   contract = name(strs1[2]); 
   if(strs1.size() == 12)
   {
     btime = utils::toInt(strs1[5]);    //红包开始时间  
     etime = utils::toInt(strs1[6]);    //红包结束时间  
     wltgroupid = utils::toInt(strs1[7]);    //白名单群组ID 
     bltgroupid = utils::toInt(strs1[8]);    //黑名单群组ID    
     customcondid = utils::toInt(strs1[9]);    //自定义条件ID，从下拉框里面选择   
     redsaetype = utils::toInt(strs1[10]);    //红包类型   1红包广场()；2个人红包(personal)； 
     random = utils::toInt(strs1[11]);   //随机1，固定2
   }

   check(num3 > 0,_error_msg(200811,"The format of the red envelope condition is incorrect"));
   check(quantity3.amount > 0,_error_msg(200331,"The amount of red envelope is incorrect!"));
   if(redsaelist_itr->redsaetype == 2)
   {
      std::string secret1 = _get_secretpwd(redid,sender.value,quantity1.amount,num1,acttime,Secret_server);     
      std::string secret2 = _get_secretpwd(owner.value,redid,0,0,0,Secret_client+secret1);
      check(secret == secret2,_error_msg(201003,"Wrong verify parameters  03!!!"));   
   }

   if(redsaelist_itr->redsaetype == 3)
   {
      asset userbalance = utils::get_balance(contract,owner,quantity1.symbol.code());
      check(userbalance.amount == 0, _error_msg(201103,"Users who have owned the token cannot participate in the red envelope grab"));

      newdtl_index _newdtl(get_self(), quantity1.symbol.code().raw());
      auto newdtl_itr = _newdtl.find(owner.value);
      check(newdtl_itr == _newdtl.end(), _error_msg(200351,"You can't grab red envelopes again" ));       
      if(newdtl_itr == _newdtl.end())
      {
         _newdtl.emplace(player, [&](auto& p) {
               p.owner = owner;
            });  
      } 

      customcondid = 0;
   }

   if(btime > 0)
   {
      check(btime<ntime,_error_msg(200603,"The time to grab the red envelope has not come  "));   
   }   
   if(etime > 0)
   {
      check(etime>ntime,_error_msg(200604,"The time to grab the red envelope has expired   "));   
   }  

   if(wltgroupid > 0)
   {
      if(wltgroupid > 10000)
      {
         wltdtl_index _wltdtl(get_self(),wltgroupid);
         auto wltdtl_itr = _wltdtl.find(owner.value);
         check(wltdtl_itr != _wltdtl.end(), _error_msg(200411,"It is not in the white list of the current red envelope and cannot rob the current red envelope!!!" ));      
      }
      else
      {
         bool ismember = _get_member(8001,owner);
         check(ismember, _error_msg(300101,"Member record does not exist! "));                        
      }
   }

   if(bltgroupid > 0)
   {
      wltdtl_index _bltdtl(get_self(),bltgroupid);
      auto bltdtl_itr = _bltdtl.find(owner.value);
      check(bltdtl_itr == _bltdtl.end(), _error_msg(200412,"In the blacklist of current red envelopes, you cannot rob the current red envelopes" ));   
   } 

   if(customcondid > 0)
   {
      isOK = false;
      customcond_index _customcond(get_self(), sender.value);
      auto customcond_itr = _customcond.find(customcondid);
      if(customcond_itr != _customcond.end())
      {
         if(customcond_itr->states == 1)
         {
            std::vector<std::string> strs2;
            strs2 = utils::split(customcond_itr->customize, "|");
            check(strs2.size() == 6,_error_msg(202011,"The custom condition is not in the correct format"));
            //代币|交易所|交易对|方向|预留|预留
            asset limitquantity = customcond_itr->quantity;
            asset swap_balance = customcond_itr->quantity;
            swap_balance.amount = 0;
            uint64_t tokenid_limit = utils::toInt(strs2[0]);
            uint64_t swapid = utils::toInt(strs2[1]);
            uint64_t pairid = utils::toInt(strs2[2]);
            uint8_t direction = utils::toInt(strs2[3]);
            //uint64_t num = utils::toInt(strs2[3]);            

            //check(false,std::to_string(customcond_itr->customtype));

            //门槛类型:1表示不限制，2表示余额红包，3表示做市红包，4表示存款红包，5表示投票红包，6表示会员红包，
            switch(customcond_itr->customtype)
            {
               case 1:
               {
                  isOK = false;
                  break;
               }
               case 2:
               {       
                  name contract_limit = _get_contract(tokenid_limit);  
                  asset userbalance = utils::get_balance(contract_limit,owner,limitquantity.symbol.code());
                  check(userbalance.amount != -1, _error_msg(201102,"Only users who have owned the token can participate in the red envelope grab"));
                  if(direction == 1)
                  {
                     check(userbalance.amount<=limitquantity.amount, _error_msg(201101,"Balance threshold red envelope, the balance does not meet the red envelope conditions  "));
                  }
                  else
                  {
                     check(userbalance.amount>=limitquantity.amount, _error_msg(201101,"Balance threshold red envelope, the balance does not meet the red envelope conditions  "));                
                  }
                  isOK = true;
                  break;
               }
               case 3:
               {
                  switch(swapid)
                  {
                     case 1://DFS
                     {
                        swap_balance =  utils::dfs_get_balance(owner,limitquantity,pairid);       
                        break;                  
                     }
                     case 2://BOX
                     {
                        swap_balance =  utils::box_get_balance(owner,limitquantity,pairid);     
                        break;                
                     }
                     case 3://OB
                     {
                        swap_balance =  utils::ob_get_balance(owner,limitquantity,pairid);   
                        break;                  
                     }
                  }  
                  check(swap_balance.amount>=limitquantity.amount, _error_msg(201201,"Market making threshold red envelope, market making does not meet the red envelope conditions "));      
                  isOK = true;                   
                  break;   
               } 
               case 6:
               {                          
                  isOK = _get_member(8001,owner);
                  check(isOK, _error_msg(300101,"Member record does not exist! "));     
                  break;   
               }                       
            }
         }
         else
         {
             _log(REDPACKET_OPEN,owner,std::to_string(redid)+"||"+std::to_string(customcondid)+"||Threshold condition status invalid"); 
            isOK = true;
         }
      }
      else
      {
         _log(REDPACKET_OPEN,owner,std::to_string(redid)+"||"+std::to_string(customcondid)+"||Threshold condition does not exist"); 
         isOK = true;
      }
   }


  if(isOK)
  {
      std::string memo = sender.to_string() + "'s red envelope !";
      asset quantity = quantity3;
      if(num3 > 1)
      {
         if(random == 1)
         {
            uint64_t random1 = max((ntime % 100),1);
            uint64_t random2 = max(((ntime / 10) % 100),1);
            uint64_t random3 = max((owner.value % 100),1);
            uint64_t random4 = max((acttime % 100),1);
            uint64_t random5 = max(((acttime / 10) % 100),1);
            uint64_t random6 = max((redid % 100),1);

            uint64_t random7 = (random1 + random2 + random3 + random4 + random5 + random6 + num3 + quantity1.amount + quantity2.amount) % 100;
            double random = (random1 * random2 * random3 * random4 * random5 * random6 * random7 * max(num2,1)  ) % 1000;
            random = max(random,100);
            double scale = 1 / double(num3-1);
            double amount = (double(quantity1.amount) - double(quantity2.amount)) * scale * 1.618 * random  / 1000;
            quantity.amount = amount;  
            if(quantity.amount>=quantity3.amount)
            {
               quantity.amount = quantity.amount / 2;
            } 
            check(quantity.amount > 0, _error_msg(200331,"The amount of red envelope is incorrect 02 "));          
         }
         else
         {
            quantity.amount = quantity1.amount / num1;
         }
      }
      else if(num3 == 1)
      {
         states = false;
      }

	   asset tmpQuantity;
      uint16_t tmpNum;
      redsaelist_index _redsaelist2(get_self(), get_self().value);
      auto redsaelist_itr2 = _redsaelist2.find(redid);
      check(redsaelist_itr2 != _redsaelist2.end(), _error_msg(200301,"The red envelope does not exist or is closed !!" ));  
      if(redsaelist_itr2 != _redsaelist2.end())
      {
         _redsaelist.modify(redsaelist_itr, same_payer, [&](auto& p) 
         {
            p.quantity2 += quantity;
            p.num2 += 1;
            p.states = states ;
            tmpQuantity = p.quantity2;
            tmpNum = p.num2;
         });
      }

      check(quantity1.amount>=tmpQuantity.amount, _error_msg(200353,"The amount of red envelope robbed is incorrect "));   
      check(num1>=tmpNum, _error_msg(200352,"The number of red envelopes robbed is incorrect"));   

      redsaedtl_index _redsae2(get_self(), redid);
      auto redsaedtl_itr2 = _redsae2.find(owner.value);
      check(redsaedtl_itr2 == _redsae2.end(), _error_msg(200351,"You can't grab red envelopes again" ));       
      if(redsaedtl_itr2 == _redsae2.end())
      {
         _redsae2.emplace(player, [&](auto& p) {
               p.user = owner;
               p.quantity = quantity;
               p.acttime = ntime;
            });  
         utils::inline_transfer(contract,get_self(),owner,quantity,memo);
      } 

      auto maxId = 0;
      usergrab_index _usergrab(get_self(), owner.value);
      _usergrab.emplace(player, [&](auto& p) 
      {
         p.id = max(1, _usergrab.available_primary_key());
         p.redid = redid;
         p.quantity = quantity;  
         p.acttime = acttime ;
         maxId = p.id;
      });
      if(maxId > 20){
         _usergrab.erase(_usergrab.begin());
      }
      
      //asset money = _get_money(tokenid,contract,quantity);
      //_update_total(owner,owner,2,quantity,money,ntime,tokenid);
      
      _close(owner);
   }
}

void redpacket::geturl(const name& owner,const uint64_t& redid)
{
   require_auth(owner);
   redsaelist_index _redsaelist(get_self(), get_self().value);
   auto redsaelist_itr = _redsaelist.find(redid);
   if(redsaelist_itr != _redsaelist.end())
   {
      if(redsaelist_itr->owner == owner)
      {
         std::string secret1 = _get_secretpwd(redid,owner.value,redsaelist_itr->quantity1.amount,redsaelist_itr->num1,redsaelist_itr->acttime,Secret_server);   
         check(false, _error_msg(900009,secret1));    
      }
   }  
}

void redpacket::mgrtoken(const name& controller,const uint64_t& tokenid,const symbol& sym,const name& contract,const uint8_t& mgrstates)
{
   require_auth(controller);
   bool  isadmin =  _manage(controller);
   check( isadmin, _error_msg(100101,"Cannot operate without permission!" ));    

   name player = controller; 
   uint8_t sign_add = 1;
   uint8_t sign_edit = 2;
   uint8_t sign_del = 4;
   uint64_t acttime = current_time_point().sec_since_epoch();
  
   if(mgrstates!=sign_add && mgrstates !=sign_edit && mgrstates !=sign_del)
   {
      check(false,_error_msg(200111,"Wrong status code "));
   }

   redsaetoken_index _redsaetoken(get_self(), get_self().value);
   auto redsaetoken_itr = _redsaetoken.find(tokenid);
   if(redsaetoken_itr != _redsaetoken.end())
   {
        if(mgrstates == sign_del )
        {
            _redsaetoken.erase(redsaetoken_itr);
        }
        
        if(mgrstates == sign_edit)
        {
            _redsaetoken.modify(redsaetoken_itr, same_payer, [&](auto& p) {
               p.sym = sym;
               p.contract = contract;
            });
        }
   }
   else
   {
      if(mgrstates == sign_add&&tokenid==0)
      {
         _redsaetoken.emplace(player, [&](auto& p) {
            p.tokenid = _get_id(name("token"),10000);
            p.sym = sym;
            p.contract = contract;
            p.states = 1 ;
         });  
      } 
   }
}


void redpacket::mgrtoken2(const name& controller,const uint64_t& tokenid,const uint8_t& states)
{
   require_auth(controller);
   bool  isadmin =  _manage(controller);
   check( isadmin, _error_msg(100101,"Cannot operate without permission!" ));    

   name player = controller; 
   uint64_t acttime = current_time_point().sec_since_epoch();
   redsaetoken_index _redsaetoken(get_self(), get_self().value);
   auto redsaetoken_itr = _redsaetoken.find(tokenid);
   if(redsaetoken_itr != _redsaetoken.end())
   {
      _redsaetoken.modify(redsaetoken_itr, same_payer, [&](auto& p) {
         p.states = states;
      });
   }
}


void redpacket::mgrprice(const name& controller,const uint64_t& tokenid,const double& price)
{
   require_auth(controller);
   bool  isadmin =  _manage(controller);
   check( isadmin, _error_msg(100101,"Cannot operate without permission!" ));    

   name player = controller; 
   redsaetoken_index _redsaetoken(get_self(), get_self().value);
   auto redsaetoken_itr = _redsaetoken.find(tokenid);
   check(redsaetoken_itr != _redsaetoken.end(),_error_msg(200501,"Token parameters are not obtained"));
   if(redsaetoken_itr != _redsaetoken.end())
   {
      _redsaetoken.modify(redsaetoken_itr, same_payer, [&](auto& p) {
         p.price = price;
      });
   }
}

void redpacket::mgrnft(const name& controller,const name& owner,const uint64_t& nftid,const std::string& title,const std::string& nfturl,const uint8_t& states,const uint8_t& mgrstates)
{
   require_auth(controller);
   bool  isadmin =  _manage(controller);
   check( isadmin, _error_msg(100101,"Cannot operate without permission!" ));    

   auto result = _mgrnft(get_self(),owner,nftid,title,nfturl,states,mgrstates);
}

bool redpacket::_mgrnft(const name& player,const name& owner,const uint64_t& nftid,const std::string& title,const std::string& nfturl,const uint8_t& states,const uint8_t& mgrstates)
{
   uint8_t sign_add = 1;
   uint8_t sign_edit = 2;
   uint8_t sign_del = 4;
   uint64_t acttime = current_time_point().sec_since_epoch();
  
   if(mgrstates!=sign_add && mgrstates !=sign_edit && mgrstates !=sign_del)
   {
      check(false,_error_msg(200111,"Wrong status code "));
   }

   redsaenft_index _redsaenft(get_self(), owner.value);
   auto redsaenft_itr = _redsaenft.find(nftid);
   if(redsaenft_itr != _redsaenft.end())
   {
        if(mgrstates == sign_del )
        {          
            _redsaenft.erase(redsaenft_itr);
        }
        
        if(mgrstates == sign_edit)
        {
            _redsaenft.modify(redsaenft_itr, same_payer, [&](auto& p) {
               p.title = title;
               p.nfturl = nfturl ;  
               p.states = states;
               p.acttime = acttime;                                          
            });
        }
   }
   else
   {
      if(mgrstates == sign_add&&nftid==0)
      {
         _redsaenft.emplace(player, [&](auto& p) {
            p.nftid = _get_id(name("nft"),10000);;
            p.title = title;
            p.nfturl = nfturl ;  
            p.states = 1;
            p.acttime = acttime;            
         });  
      } 
   }

   return true;
}


void redpacket::mgrmkt(const name& controller,const uint64_t& mktid, const uint64_t& swapid,const std::string& swapname,const uint64_t& pairid,const std::string& pairname,const uint8_t& states,const uint8_t& mgrstates)
{
   require_auth(controller);
   bool  isadmin =  _manage(controller);
   check( isadmin, _error_msg(100101,"Cannot operate without permission!" ));    

   name player = controller; 
   uint8_t sign_add = 1;
   uint8_t sign_edit =2;
   uint8_t sign_del = 4;
   uint64_t acttime = current_time_point().sec_since_epoch();
  
   if(mgrstates!=sign_add && mgrstates !=sign_edit && mgrstates !=sign_del)
   {
      check(false,_error_msg(200111,"Wrong status code "));
   }

   uint64_t _mktid = utils::toPosition(swapid,pairid,5);
   check( _mktid == mktid, _error_msg(201202,"Wrong primary key value !"));   

   redsaemkt_index _redsaemkt(get_self(), get_self().value);
   auto redsaemkt_itr = _redsaemkt.find(mktid);
   if(redsaemkt_itr != _redsaemkt.end())
   {
        if(mgrstates == sign_del )
        {
            _redsaemkt.erase(redsaemkt_itr);
        }
        
        if(mgrstates == sign_edit)
        {
            _redsaemkt.modify(redsaemkt_itr, same_payer, [&](auto& p) {
               p.swapid = swapid;
               p.swapname = swapname;
               p.pairid = pairid ;
               p.pairname = pairname ;
               p.acttime = acttime ;
               p.states = states;   
            });
        }
   }
   else
   {
      if(mgrstates == sign_add)
      {
         _redsaemkt.emplace(player, [&](auto& p) {
            p.mktid = mktid;
            p.swapid = swapid;
            p.swapname = swapname;
            p.pairid = pairid ;
            p.pairname = pairname ;
            p.acttime = acttime ;  
            p.states = 1;                
         });   
      }
   }
}  

void redpacket::mgrdiycond(const name& controller,const name& owner,uint64_t customcondid, const uint8_t& customtype,const std::string& title, const asset& quantity, const std::string& customize,const uint8_t& mgrstates)
{
   require_auth(controller);
   bool  isadmin =  _manage(controller);
   check( isadmin, _error_msg(100101,"Cannot operate without permission!" ));    

   auto result = _customcond(get_self(),owner,customcondid,customtype,title,quantity,customize,mgrstates);
}

void redpacket::usrdiycond(const name& owner,uint64_t customcondid,const uint8_t& customtype,const std::string& title, const asset& quantity, const std::string& customize,const uint8_t& mgrstates)
{
   require_auth(owner);
   auto result = _customcond(owner,owner,customcondid,customtype,title,quantity,customize,mgrstates);
}

void redpacket::deldiycond(const name& owner,uint64_t customcondid)
{
   require_auth(owner);

   customcond_index _customcond(get_self(), owner.value);
   auto customcond_itr = _customcond.find(customcondid);
   check(customcond_itr != _customcond.end(),_error_msg(200309,"The ID of the threshold condition is incorrect"));
   if(customcond_itr != _customcond.end())
   {
      _customcond.erase(customcond_itr);
   }
}

bool redpacket::_customcond(const name& player,const name& owner,uint64_t customcondid, const uint8_t& customtype,const std::string& title, const asset& quantity, const std::string& customize,const uint8_t& mgrstates)
{
   uint8_t sign_add = 1;
   uint8_t sign_edit =2;
   uint8_t sign_del = 4;
   uint64_t ntime = current_time_point().sec_since_epoch();   

   check( customtype <= 6, _error_msg(200812,"Incorrect threshold type !"));   

   if(mgrstates != 1 && mgrstates !=2 && mgrstates !=4 )
   {
      check(false,_error_msg(200111,"Wrong status code 1"));
   }

   check(title.size() <= 30, _error_msg(200112,"The threshold Title length cannot exceed 10 characters !"));    

   if(customcondid == 0)
   {
      check(mgrstates == sign_add,_error_msg(200111,"Wrong status code 2"));
      customcondid = _get_id(name("customcond"),10000);
   }

   customcond_index _customcond(get_self(), owner.value);
   auto customcond_itr = _customcond.find(customcondid);
   if(customcond_itr != _customcond.end())
   {
        if(mgrstates == sign_del )
        {
            _customcond.erase(customcond_itr);
        }
        if(mgrstates == sign_edit )
        {
            _customcond.modify(customcond_itr, same_payer, [&](auto& p) {
               p.customtype = customtype;
               p.title = title ;
               p.quantity = quantity ;
               p.customize = customize ;
               p.acttime = ntime;  
            });
        }
   }
   else
   {
      if(mgrstates == sign_add )
      {
         _customcond.emplace(player, [&](auto& p) {
            p.customcondid = customcondid;
            p.customtype = customtype;
            p.title = title ;
            p.quantity = quantity ;
            p.customize = customize ;            
            p.states = 1; 
            p.acttime = ntime;                          
         }); 
      }  
   }

   return true;
}

void redpacket::mgrcustomize(const name& controller,const name& owner,const uint64_t& customcondid,const asset& quantity,const std::string& customize)
{
    require_auth(controller);
    bool  isadmin =  _manage(controller);
    check( isadmin, _error_msg(100101,"Cannot operate without permission!" ));    

   auto result = _customize(owner,customcondid,quantity,customize);
}


void redpacket::usrcustomize(const name& owner,const uint64_t& customcondid,const asset& quantity,const std::string& customize)
{
    require_auth(owner);
    auto result = _customize(owner,customcondid,quantity,customize);
}


bool redpacket::_customize(const name& owner,const uint64_t& customcondid,const asset& quantity,const std::string& customize)
{

   check(customcondid >= 0,_error_msg(200110,"Bad ID "));

   std::vector<std::string> strs;
   strs = utils::split(customize, "|");
   check(strs.size() == 6,_error_msg(202011,"The custom condition is not in the correct format"));

   customcond_index _customcond(get_self(), owner.value);
   auto customcond_itr = _customcond.find(customcondid);
   if(customcond_itr != _customcond.end())
   {
      _customcond.modify(customcond_itr, same_payer, [&](auto& p) {
         p.quantity = quantity;
         p.customize = customize; 
      });
   }
   return true;
}


void redpacket::mgrwltgroup(const name& controller,const name& owner,const uint64_t& groupid,const std::string& title,const std::string& content,const uint8_t& states,const uint8_t& mgrstates)
{
   require_auth(controller);
   bool  isadmin =  _manage(controller);
   check( isadmin, _error_msg(100101,"Cannot operate without permission!" ));    

   _wltgroup(controller,owner,groupid,title,content,states,mgrstates);
}

void redpacket::userwltgroup(const name& owner,const uint64_t& groupid,const std::string& title,const std::string& content,const uint8_t& states,const uint8_t& mgrstates)
{
   require_auth(owner);  
   _wltgroup(owner,owner,groupid,title,content,states,mgrstates);
}


void redpacket::_wltgroup(const name& player,const name& owner,const uint64_t& groupid,const std::string& title,const std::string& content,const uint8_t& states,const uint8_t& mgrstates)
{
   uint8_t sign_add = 1;
   uint8_t sign_edit = 2;
   uint8_t sign_del = 4;
   uint64_t ntime = current_time_point().sec_since_epoch();
  
   if(mgrstates!=sign_add && mgrstates !=sign_edit && mgrstates !=sign_del)
   {
      check(false,_error_msg(200111,"Wrong status code "));
   }
   
   wltgroup_index _wltgroup(get_self(), owner.value);
   auto wltgroup_itr = _wltgroup.find(groupid);
   if(wltgroup_itr != _wltgroup.end())
   {
        if(mgrstates == sign_del )
        {
          
            _wltgroup.erase(wltgroup_itr);
            
            wltdtl_index _wltdtl(get_self(), groupid);
            std::vector<uint64_t> keysForDeletion;  
            for(auto& item : _wltdtl) {  
               keysForDeletion.push_back(item.user.value );  
            }  
            for (uint64_t key : keysForDeletion) {  
               auto itr = _wltdtl.find(key);  
               if (itr != _wltdtl.end()) {  
                     _wltdtl.erase(itr);  
               }  
            } 
        }
        
        if(mgrstates == sign_edit)
        {
            _wltgroup.modify(wltgroup_itr, same_payer, [&](auto& p) {
               p.groupid = groupid;
               p.title = title;
               p.content = title;
               p.states = states ;
               p.acttime = ntime;              
            });
        }
   }
   else
   {
      if(mgrstates == sign_add&&groupid==0)
      {
         _wltgroup.emplace(player, [&](auto& p) {
            p.groupid = _get_id(name("wltgroup"),10000);
            p.title = title;
            p.content = content;
            p.states = 1 ;
            p.acttime = ntime; 
         });  
      } 
   }

}


void redpacket::userwltcls(const name& owner,const uint64_t& groupid)
{
   require_auth(owner);   
   wltgroup_index _wltgroup(get_self(), owner.value);
   auto wltgroup_itr = _wltgroup.find(groupid);
   check(wltgroup_itr != _wltgroup.end(), _error_msg(200401,"Group record does not exist!" ));      
   if(wltgroup_itr != _wltgroup.end())
   {
      wltdtl_index _wltdtl(get_self(), groupid);
      std::vector<uint64_t> keysForDeletion;  
      for(auto& item : _wltdtl)
      {  
         keysForDeletion.push_back(item.user.value );  
      }  
      for (uint64_t key : keysForDeletion) 
      {  
         auto itr = _wltdtl.find(key);  
         if (itr != _wltdtl.end()) 
         {  
            _wltdtl.erase(itr);  
         }  
      } 
   }
}


void redpacket::mgrwltdtl(const name& controller,const name& owner,const uint64_t& groupid,const name& user,const uint8_t& mgrstates)
{
   require_auth(controller);
   bool  isadmin =  _manage(controller);
   check( isadmin, _error_msg(100101,"Cannot operate without permission!" ));    

   _wltdtl(controller,owner,groupid,user,mgrstates);
}

void redpacket::userwltdtl(const name& owner,const uint64_t& groupid,const name& user,const uint8_t& mgrstates)
{
   require_auth(owner);   
   _wltdtl(owner,owner,groupid,user,mgrstates);

}

void redpacket::_wltdtl(const name& player,const name& owner,const uint64_t& groupid,const name& user,const uint8_t& mgrstates)
{
   uint8_t sign_add = 1;
   uint8_t sign_del = 4;
   uint64_t acttime = current_time_point().sec_since_epoch();
  
   if(mgrstates!=sign_add  && mgrstates !=sign_del)
   {
      check(false,_error_msg(200111,"Wrong status code "));
   }

   wltdtl_index  _wltdtl(get_self(), groupid);
   auto wltdtl_itr = _wltdtl.find(user.value);
   if(wltdtl_itr != _wltdtl.end())
   {
        if(mgrstates == sign_del )
        {          
            _wltdtl.erase(wltdtl_itr);
        }
   }
   else
   {
      if(mgrstates == sign_add)
      {
         _wltdtl.emplace(player, [&](auto& p) {
            p.user = user;
            p.acttime = acttime ;
         });  
      } 
   }
}


void redpacket::closeredsae(const name& owner,const uint64_t& redid)
{
   require_auth(owner);
   action(
        permission_level{get_self(), "active"_n},
        get_self(),
        name("mgrclosered"),
        std::make_tuple(get_self(),owner,redid))
        .send();

   action(
        permission_level{get_self(), "active"_n},
        get_self(),
        name("mgrdeletered"),
        std::make_tuple(get_self(),owner,redid))
        .send();        
}


void redpacket::mgrclear(const name& owner,const uint64_t& redid)
{
   require_auth(get_self());
   action(
        permission_level{get_self(), "active"_n},
        get_self(),
        name("mgrclosered"),
        std::make_tuple(get_self(),owner,redid))
        .send();

   action(
        permission_level{get_self(), "active"_n},
        get_self(),
        name("mgrdeletered"),
        std::make_tuple(get_self(),owner,redid))
        .send();        
}

void redpacket::mgrclosered(const name& controller,const name& owner,const uint64_t& redid)
{
    require_auth(controller);
    bool  isadmin =  _manage(controller);
    check( isadmin, _error_msg(100101,"Cannot operate without permission!" ));    

   _closeredsae(false,get_self(),owner,redid);
}

void redpacket::_closeredsae(const bool& isauth,const name& player,const name& owner,const uint64_t& redid)
{
   uint64_t ntime =  current_time_point().sec_since_epoch();
   //uint64_t ntime2 = ntime - 259200;
   //uint64_t ntime2 = ntime - 0;
   redsaelist_index _redsaelist(get_self(), get_self().value);
   auto redsaelist_itr = _redsaelist.find(redid);
   //check(redsaelist_itr != _redsaelist.end(), _error_msg(200301,"The red envelope does not exist or is closed" ));     
   //check(redsaelist_itr->states,_error_msg(200303,"The red envelope campaign has ended" ));   
   //check(redsaelist_itr->acttime <= ntime2, _error_msg(200605,"The red envelope creation time is less than three days and cannot be closed" ));       
   if(isauth)
   {
      check(redsaelist_itr->owner == owner, _error_msg(201010,"You are not the sender of the current red envelope" ));
   } 
   if(redsaelist_itr != _redsaelist.end())
   {
      _redsaelist.modify(redsaelist_itr, same_payer, [&](auto& p) {
         p.states = false ;
         p.acttime = ntime;
      });   
   }
}

void redpacket::mgrdeletered(const name& controller,const name& owner,const uint64_t& redid)
{
    require_auth(controller);
    bool  isadmin =  _manage(controller);
    check( isadmin, _error_msg(100101,"Cannot operate without permission!" ));    

   _deleteredsae(false,get_self(),owner,redid);
}

void redpacket::_deleteredsae(const bool& isauth,const name& player,const name& owner,const uint64_t& redid)
{
   //uint64_t ntime =  current_time_point().sec_since_epoch();
   redsaelist_index _redsaelist(get_self(), get_self().value);
   auto redsaelist_itr = _redsaelist.find(redid);
   check(redsaelist_itr != _redsaelist.end(), _error_msg(200301,"The red envelope does not exist or is closed" ));  
   check(!redsaelist_itr->states,_error_msg(200303,"The red envelope campaign has ended" ));  
   if(isauth)
   {
      check(redsaelist_itr->owner == owner, _error_msg(201010,"You are not the sender of the current red envelope" ));
   }    
   if(redsaelist_itr != _redsaelist.end())
   { 
      std::vector<std::string> strs1;
      strs1 = utils::split(redsaelist_itr->condition, "|");
      name     contract = name(strs1[2]); //代币合约
      asset quantity = redsaelist_itr->quantity1 - redsaelist_itr->quantity2;   

      asset quantity_fee = quantity * 2 / 10;
      asset quantity_out = quantity - quantity_fee;
      check(quantity.amount >= 0, _error_msg(200331,"The amount of red envelope is incorrect"));   

      _redsaelist.erase(redsaelist_itr);

      redsaedtl_index _redsae(get_self(), redid);
      std::vector<uint64_t> keysForDeletion;  
      for(auto& item : _redsae) {  
            keysForDeletion.push_back(item.user.value );  
      }  
      for (uint64_t key : keysForDeletion) {  
         auto itr = _redsae.find(key);  
         if (itr != _redsae.end()) {  
            _redsae.erase(itr);  
         }  
      } 
      
      if(quantity_out.amount > 0)
      {
          utils::inline_transfer(contract,get_self(),owner,quantity_out,"Red Packet Closed ");   
      }  
      if(quantity_fee.amount > 0)
      {
          utils::inline_transfer(contract,get_self(),REDPACKET_FEE,quantity_fee,"Red Packet Closed ");  
      }
   }
}

void redpacket::mgrtest(const name& owner,const uint64_t& redid)
{
   require_auth(get_self());

   redsaelist_index _redsaelist(get_self(), get_self().value);
   auto redsaelist_itr = _redsaelist.find(redid);
  	check(redsaelist_itr != _redsaelist.end(), _error_msg(200301,"The red envelope does not exist or is closed !!" ));    

   std::string verify1 = _get_secretpwd(owner.value,redid,0,0,0,Secret_client);
   std::string secret1 = _get_secretpwd(redid,redsaelist_itr->owner.value,redsaelist_itr->quantity1.amount,redsaelist_itr->num1,redsaelist_itr->acttime,Secret_server);     
   std::string secret2 = _get_secretpwd(owner.value,redid,0,0,0,Secret_client+secret1);

   std::vector<std::string> sentence;
   sentence.push_back("0");
   sentence.push_back("owner.value");
   sentence.push_back(std::to_string(owner.value));

   sentence.push_back("0");
   sentence.push_back("id");
   sentence.push_back(std::to_string(redid));   

   sentence.push_back("1");
   sentence.push_back("Secret_client");
   sentence.push_back(Secret_client);

   sentence.push_back("1");
   sentence.push_back("verify1");
   sentence.push_back(verify1);


   sentence.push_back("0");
   sentence.push_back("quantity1.amount");
   sentence.push_back(std::to_string(redsaelist_itr->quantity1.amount));

   sentence.push_back("0");
   sentence.push_back("num1");
   sentence.push_back(std::to_string(redsaelist_itr->num1));

   sentence.push_back("0");
   sentence.push_back("acttime");
   sentence.push_back(std::to_string(redsaelist_itr->acttime));

   sentence.push_back("1");
   sentence.push_back("Secret_server");
   sentence.push_back(Secret_server);   

   sentence.push_back("1");
   sentence.push_back("secret1");
   sentence.push_back(secret1);

   sentence.push_back("1");
   sentence.push_back("secret2");
   sentence.push_back(secret2);

   std::string result = utils::toJson(sentence);
   check(false,result);

}

void redpacket::test2(const name& owner,const uint64_t& redid)
{
   require_auth(get_self());  
   redsaedtl_index _redsaedtl(get_self(), redid);
   auto redsaedtl_itr = _redsaedtl.find(owner.value);
	check(redsaedtl_itr != _redsaedtl.end(), _error_msg(200351,"You can't grab red envelopes again!" ));   
   _redsaedtl.erase(redsaedtl_itr);  
}

void redpacket::test3(const name& owner,const uint64_t& redid)
{ 
   require_auth(get_self());  
   redsaelist_index _redsaelist(get_self(), get_self().value);
   auto redsaelist_itr = _redsaelist.find(redid);
   if(redsaelist_itr->redsaetype==3)
   {
      newdtl_index _newdtl(get_self(), redsaelist_itr->quantity1.symbol.code().raw());
      auto newdtl_itr = _newdtl.find(owner.value); 
      if(newdtl_itr != _newdtl.end())
      {
         _newdtl.erase(newdtl_itr);  
      } 
   }
}

void redpacket::test4(const name& owner,const symbol& sym)
{ 
   require_auth(get_self());  
   newdtl_index _newdtl(get_self(), sym.code().raw());
   auto newdtl_itr = _newdtl.find(owner.value); 
   if(newdtl_itr != _newdtl.end())
   {
      _newdtl.erase(newdtl_itr);  
   } 
}

void redpacket::_update_total(const name& owner, const name& player, const uint64_t& saletype,const asset& quantity,const asset& money, const uint64_t& ntime,const uint64_t& tokenid)
{
   uint64_t month = utils::getym(ntime);

   uint64_t socpetotal = saletype * 100000000000LL + month * 100000 + tokenid;

   redsaetotal_index _redsaetotal(get_self(), socpetotal);
   auto redsaetotal_itr = _redsaetotal.find(owner.value);
   if(redsaetotal_itr == _redsaetotal.end())
   {
      _redsaetotal.emplace(player, [&](auto& p) {
         p.owner = owner;
         p.num = 1;
         p.quantity = quantity;
         p.money = money;
         p.acttime = ntime;
      });  
   }
   else
   {
      _redsaetotal.modify(redsaetotal_itr, same_payer, [&](auto& p) 
      {
         p.num += 1;
         p.quantity += quantity;
         p.money += money;
         p.acttime = ntime ;
      });
   }
}

bool redpacket::_get_customcond(const uint64_t& customcondid,const name& owner)
{
   bool isok = false;
   if(customcondid > 0)
   {
      isok = true;
      customcond_index _customcond(get_self(), owner.value);
      auto customcond_itr = _customcond.find(customcondid);
      check(customcond_itr != _customcond.end(),_error_msg(202001,"The record of custom condition does not exist"));    
   }
   else if(customcondid < 0)
   {
      check(false,_error_msg(200110,"Bad ID  !!!"));     
   }
   return isok;
}

bool redpacket::_get_wlt_group(const uint64_t& wltgroupid,const name& owner)
{
  bool isok = false;
  if(wltgroupid > 0)
   {
      isok = true;
      if(wltgroupid > 10000)
      {
         wltgroup_index _wltgroup(get_self(), owner.value);
         auto wltgroup_itr = _wltgroup.find(wltgroupid);
         check(wltgroup_itr != _wltgroup.end(),_error_msg(200401,"Group record does not exist!   !("+std::to_string(wltgroupid)+")"));     
      } 
   }
   else if(wltgroupid < 0)
   {
      check(false,_error_msg(200110,"Bad ID !!!"));     
   }
   return isok;
}


std::string redpacket::_get_nft_url(const uint64_t& nftid,const name& owner)
{
   std::string nfturl="";
   if(nftid > 0)
   {
      redsaenft_index _redsaenft(get_self(), owner.value);
      auto redsaenft_itr = _redsaenft.find(nftid);
      check(redsaenft_itr != _redsaenft.end(),_error_msg(200701,"NFT record does not exist  !("+std::to_string(nftid)+")"));      
      nfturl = redsaenft_itr->nfturl;
   }
   else if(nftid < 0)
   {
      check(false,_error_msg(200110,"Bad ID   !!!"));     
   }
   return nfturl;
}

asset redpacket::_get_money(const uint64_t& tokenid,const name& contract,const asset& quantity)
{
   asset money =  asset(0, symbol("USDT",4));  
   if(tokenid > 0)
   {
      redsaetoken_index _redsaetoken(get_self(), get_self().value);
      auto redsaetoken_itr = _redsaetoken.find(tokenid);
      check(redsaetoken_itr != _redsaetoken.end(),_error_msg(200501,"Token parameters are not obtained  !("+std::to_string(tokenid)+")"));     
      check(redsaetoken_itr->contract == contract,_error_msg(200511,"Incorrect token contract "));
      money.amount = quantity.amount * redsaetoken_itr->price;
   }
   else if(tokenid <= 0)
   {
      check(false,_error_msg(200512,"Wrong token parameters  !!!"));     
   }
   return money;
}


name redpacket::_get_contract(const uint64_t& tokenid)
{
   name contract;
   if(tokenid > 0)
   {
      redsaetoken_index _redsaetoken(get_self(), get_self().value);
      auto redsaetoken_itr = _redsaetoken.find(tokenid);
      if(redsaetoken_itr != _redsaetoken.end())
      {
          contract = redsaetoken_itr->contract;
      }
   }
   return contract;
}


bool redpacket::_get_member(const uint64_t& memberid,const name& owner)
{
  bool isok = true;
  uint64_t ntime = current_time_point().sec_since_epoch();
  switch(memberid)
  {
     case 8001:
          isok = utils::pink_get_member(CONTRACT_MEMBER_PINK,owner,ntime);
          break;
     default:
          isok = false;
          break;
  }
  return isok;
}



std::string redpacket::_get_secretpwd(uint64_t random1,uint64_t random2,uint64_t random3,uint64_t random4,uint64_t random5,std::string secretkey)
{
   uint64_t verify = (random1 + random2 + random3 + random4 + random5) % 1000000;
   std::string  secret =std::to_string(verify)+secretkey;
   const char *verify_cstr = secret.c_str();
   checksum256 digest = sha256(verify_cstr, strlen(verify_cstr));
   return utils::sha256_to_hex(digest);
}


uint64_t redpacket::_get_id(const name& socpename,const uint64_t& minid)
{
   globals_index _globals(get_self(), get_self().value);
   auto itr = _globals.find(socpename.value);
   uint64_t id = minid;  
   if (itr != _globals.end())
   {
      id = itr->val + 1;
      _globals.modify(itr, same_payer, [&](auto &s) {
         s.val = id;
      });
   }
   else
   {
      _globals.emplace(get_self(), [&](auto &s) {
         s.key = socpename;
         s.val = id;
      });
   }
   return id;
}

bool redpacket::_manage(const name &account)
{
   if(account == REDPACKET_MGR || account == get_self())
   {
      return true;
   }
   return false;
} 


void redpacket::_close(const name& owner)
{
    uint64_t redid = 0;
    redsaelist_index _redsaelist(get_self(), get_self().value);
    auto states_table = _redsaelist.get_index<name("states")>();
    auto itr_begin = states_table.lower_bound(0);

    for(; itr_begin != states_table.end() && itr_begin->states == 0; ++itr_begin)
    {
         redid = itr_begin->redid;
         states_table.erase(itr_begin);  
         _log(get_self(),owner,std::to_string(redid)+"||del21");     
         break;     
    }    
} 

std::string redpacket::_error_msg(const uint64_t& code,const std::string& msg)
{
   std::vector<std::string> error;
   error.push_back("0");
   error.push_back("code");
   error.push_back(std::to_string(code));
                  
   error.push_back("1");
   error.push_back("msg");
   error.push_back(msg);

	return utils::toJson(error);
}

void redpacket::_log(const name& scope, const name& owner, const std::string& opevent) {
	auto maxId = 0;

	logs_index logs(get_self(), scope.value);

	logs.emplace(get_self(), [&](auto& p) {
		p.id = max(1, logs.available_primary_key());
		p.owner = owner;
		p.opevent = opevent;
		p.acttime = current_time_point().sec_since_epoch() ;
		maxId = p.id;
		});
	if(maxId > 100){
		logs.erase(logs.begin());
	}		
}
