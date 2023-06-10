#include <types.hpp>
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/action.hpp>
#include <eosio/contract.hpp>
#include <eosio/crypto.hpp>
#include <eosio/ignore.hpp>
#include <ctime>
#include <string>

namespace utils
{
    using std::string;
    using eosio::extended_symbol;

    void inline_transfer(name contract, name from, name to, asset quantity, string memo)
    {
      action(
            permission_level{from, "active"_n},
            contract,
            name("transfer"),
            make_tuple(from, to, quantity, memo))
        .send();
    }

   
    std::string toJson(std::vector<std::string> sentence)
    {
        std::string result ="{" ;
        int m = 0 ;
        int n = sentence.size();
        bool isadd = false ;
        for(int i=0;i< n;i++)
        {
            m = (i + 1) % 3;
            if(m==1)
            {
               if(sentence[i]=="1")
               {
                  isadd = true;                  
               }
               else
               {
                  isadd = false;        
               }
            }
            else if(m == 2)
            {
                result = result +"\"" +sentence[i] +"\":";
            }
            else if(m == 0)
            {
                if(isadd)
                {
                    result = result +"\"" + sentence[i] +"\"";
                }
                else
                {
                    result = result + sentence[i] ;              
                }
                if((i + 1)!=n)
                {
                    result = result + ",";
                }
            }
        }
        return  result + "}";
    }  

    uint64_t toInt(const std::string& str) 
    {
        if (str.empty() || str == "") {
            return 0;
        }
        else {
            std::string::size_type sz = 0;
            return std::stoull(str, &sz, 0);
        }
    }     

    std::vector<string> split(const string &str, const string &delim)
    {
      std::vector<string> strs;
      size_t prev = 0, pos = 0;
      do
      {
         pos = str.find(delim, prev);
         if (pos == string::npos)
            pos = str.length();
         string s = str.substr(prev, pos - prev);
         if (!str.empty())
            strs.push_back(s);
         prev = pos + delim.length();
      } while (pos < str.length() && prev < str.length());
      return strs;
    }


    uint64_t toPosition(const uint64_t& num1,const uint64_t& num2,const uint64_t& places2) 
    {
       uint64_t num = 0;
       std::string str1 = std::to_string(num2);
       std::string str2;
       std::string str3;
       uint64_t diff = places2 - str1.size();       
       if(diff < 0)
       {
          return num;
       }

       for(int i = 0 ;i < diff; i++)
       {
          str2 = str2 +"0";
       }
       str3 = std::to_string(num1)+str2+str1;
       num = toInt(str3);
       return num;
    }  

   asset get_supply(const name &token_contract_account, const symbol_code &sym_code)
   {
      stats_index statstable(token_contract_account, sym_code.raw());
      string err_msg = "invalid token contract: ";
      err_msg.append(token_contract_account.to_string());
      const auto &st = statstable.require_find(sym_code.raw(), err_msg.c_str());
      return st->supply;
   }

   asset get_balance(const name &token_contract_account, const name &owner, const symbol_code &sym_code)
   {
      accounts_index accountstable(token_contract_account, owner.value);

      auto itr = accountstable.find(sym_code.raw());
      if (itr == accountstable.end())
      {
         asset supply = get_supply(token_contract_account, sym_code);
         return asset(-1, supply.symbol);
      }
      else
      {
         return itr->balance;
      }
   }  

   bool is_newuser(const name &token_contract_account, const name &owner, const symbol_code &sym_code)
   {
      accounts_index accountstable(token_contract_account, owner.value);

      auto itr = accountstable.find(sym_code.raw());
      if (itr == accountstable.end())
      {
         return true;
      }
      else
      {
         return false;
      }
   }   

   asset dfs_get_balance(const name &owner,const asset &limitquantity,const uint64_t& pairid)
   {
      asset balance = limitquantity;
      balance.amount = 0;
      uint64_t liquidity_token = 0;
      uint64_t liquidity_user = 0;
      double amount = 0;

      dfs_liqs2 _liqs2(DFS_SLOGSONE,owner.value);
      auto liqs2_itr = _liqs2.find(pairid);
      if (liqs2_itr != _liqs2.end())
      {
         liquidity_user =  liqs2_itr->token ;//用户做市凭证
         if(liquidity_user > 0)
         {
            dfs_markets _markets(DFS_SSWAPCNT,DFS_SSWAPCNT.value);
            auto market_itr = _markets.find(pairid);
            if (market_itr != _markets.end())
            {
               liquidity_token = market_itr->liquidity_token;//总的做市凭证

               if(limitquantity.symbol == market_itr->reserve0.symbol)
               {
                  amount = double(market_itr->reserve0.amount) * double(liquidity_user) / double(liquidity_token);
                  balance.amount = uint64_t(amount);
               }                               
               if(limitquantity.symbol == market_itr->reserve1.symbol)
               {
                  amount = double(market_itr->reserve1.amount) * double(liquidity_user) / double(liquidity_token);
                  balance.amount = uint64_t(amount);                    
               } 
            }     
         }

      }

      return balance;
   } 


   static symbol box_get_lptoken(uint64_t pair_id )
   {
      if(pair_id == 0) return {};
      std::string res;
      while(pair_id){
         res = (char)('A' + pair_id % 26 - 1) + res;
         pair_id /= 26;
      }
      return symbol { eosio::symbol_code{ "BOX" + res }, 0 };
   }

   asset box_get_balance(const name &owner,const asset &limitquantity,const uint64_t& pairid)
   {
      asset balance = limitquantity;
      balance.amount = 0;
      uint64_t liquidity_token = 0;
      uint64_t liquidity_user = 0;
      double amount = 0;

      auto lptoken = box_get_lptoken(pairid);

      box_user_info _userinfo(BOX_LPTOKEN,lptoken.code().raw());      
      auto user_info_itr = _userinfo.find(owner.value);
      if (user_info_itr != _userinfo.end())
      {
         liquidity_user = user_info_itr->liquidity;

         if(liquidity_user > 0)
         {
            box_pairs _pairs(BOX_SWAP,BOX_SWAP.value);
            auto pairs_itr = _pairs.find(pairid);
            if (pairs_itr != _pairs.end())
            {
               liquidity_token = pairs_itr->liquidity_token;       
               if(limitquantity.symbol == pairs_itr->reserve0.symbol)
               {
                  amount = double(pairs_itr->reserve0.amount) * double(liquidity_user) / double(liquidity_token);
                  balance.amount = uint64_t(amount);
               }                               
               if(limitquantity.symbol == pairs_itr->reserve1.symbol)
               {
                  amount = double(pairs_itr->reserve1.amount) * double(liquidity_user) / double(liquidity_token);
                  balance.amount = uint64_t(amount);  
               } 
            }
         }
      }
      return balance;
   } 


   asset ob_get_balance(const name &owner,const asset &limitquantity,const uint64_t& pairid)
   {
      asset balance = limitquantity;
      balance.amount = 0;
      uint64_t liquidity_token = 0;
      uint64_t liquidity_user = 0;
      double amount = 0;

      ob_liquidity _liquidity(OB_SWAP,owner.value);      
      auto liquidity_itr = _liquidity.find(pairid);
      if (liquidity_itr != _liquidity.end())
      {
         liquidity_user = liquidity_itr->token;

         if(liquidity_user > 0)
         {
            ob_pair _pairs(OB_SWAP,OB_SWAP.value);
            auto pairs_itr = _pairs.find(pairid);
            if (pairs_itr != _pairs.end())
            {
               liquidity_token = pairs_itr->liquidity_token;       
               if(limitquantity.symbol == pairs_itr->reserve0.symbol)
               {
                  amount = double(pairs_itr->reserve0.amount) * double(liquidity_user) / double(liquidity_token);
                  balance.amount = uint64_t(amount);
               }                               
               if(limitquantity.symbol == pairs_itr->reserve1.symbol)
               {
                  amount = double(pairs_itr->reserve1.amount) * double(liquidity_user) / double(liquidity_token);
                  balance.amount = uint64_t(amount);  
               } 
            }
         }
      }
      return balance;
   } 


   string to_hex(const char *d, uint32_t s)
   {
      std::string r;
      const char *to_hex = "0123456789abcdef";
      uint8_t *c = (uint8_t *)d;
      for (uint32_t i = 0; i < s; ++i)
         (r += to_hex[(c[i] >> 4)]) += to_hex[(c[i] & 0x0f)];
      return r;
   }

   string sha256_to_hex(const checksum256 &sha256)
   {
      auto hash_data = sha256.extract_as_byte_array();
      return to_hex((char *)hash_data.data(), sizeof(hash_data.data()));
   }

   uint64_t uint64_hash(const string &hash)
   {
      return std::hash<string>{}(hash);
   }

   uint64_t uint64_hash(const checksum256 &hash)
   {
      return uint64_hash(sha256_to_hex(hash));
   }

   bool pink_get_member(const name &scope,const name &owner,const uint64_t& ntime)
   {
      bool isok = false;
      pink_members _members(scope, scope.value);
      auto members_itr = _members.find(owner.value);
      if(members_itr != _members.end())
      {
         if(members_itr->exptime >= ntime)
         {
            isok = true;
         }
      }
      return isok;  
   } 

   uint32_t getym(uint64_t time)
   {
      uint32_t days = time / 86400 + 1;
      uint16_t year = 1970;
      uint8_t month = 0;
      uint16_t day = 0;
      uint8_t mdays[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
      while (true) 
      {
         bool isLeap = year % 400 == 0 || (year % 4 == 0 && year % 100 != 0);
         if (days >= 365) 
         {
            days -= isLeap ? 366 : 365;
            year++;
         } 
         else 
         {
            mdays[1] = isLeap ? 29 : 28;
            for (uint8_t i = 0; i < 12; i++) 
            {
               if (days > mdays[i]) 
               {
                  days -= mdays[i];
               } else 
               {
                  month = i + 1;
                  break;
               }
            }
            break;
         }
      }
      return year * 100 + month;
   }

   uint32_t getymd(uint64_t time)
   {
      uint32_t days = time / 86400 + 1;
      uint16_t year = 1970;
      uint8_t month = 0;
      uint16_t day = 0;
      uint8_t mdays[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
      while (true) 
      {
         bool isLeap = year % 400 == 0 || (year % 4 == 0 && year % 100 != 0);
         if (days >= 365) 
         {
            days -= isLeap ? 366 : 365;
            year++;
         } 
         else 
         {
            mdays[1] = isLeap ? 29 : 28;
            for (uint8_t i = 0; i < 12; i++) 
            {
               if (days > mdays[i]) 
               {
                  days -= mdays[i];
               } else 
               {
                  month = i + 1;
                  break;
               }
            }
            break;
         }
      }
      return year * 10000 + month * 100 + days;
   }

}
