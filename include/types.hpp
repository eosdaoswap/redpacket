#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/system.hpp>

using namespace eosio;

static constexpr name BOX_SWAP  { name("swap.defi") };
static constexpr name BOX_LPTOKEN  { name("lptoken.defi") };

static constexpr name DFS_SLOGSONE  { name("defislogsone") };
static constexpr name DFS_SSWAPCNT  { name("defisswapcnt") };

static constexpr name OB_SWAP  { name("swap.ob") };


static constexpr name REDPACKET_FEE  { name("feeooooooooo") };
static constexpr name REDPACKET_MGR  { name("defibox.wei") };
static constexpr name REDPACKET_OPEN {name("open")};

static constexpr name CONTRACT_MEMBER_PINK {name("pinkercenter")};


static const std::string Secret_server = "uNo4ms0J";
static const std::string Secret_client = "KshgJC5G";

//static const std::string Secret_server = "uNo4ms0J";
//static const std::string Secret_client = "KshgJC5G";

static const uint64_t Money_min = 1000;
//static const uint64_t Money_max = 100000 * 10000 ;
static const uint64_t Redpacket_tax = 3;

struct  Condition 
{     
    std::string       title;  
    std::string       content;  
          asset       quantity;                                        
};

struct [[eosio::table]] currency_stats
{
    asset supply;
    asset max_supply;
    name issuer;

    uint64_t primary_key() const { return supply.symbol.code().raw(); }
};
typedef eosio::multi_index<"stat"_n, currency_stats> stats_index;

struct [[eosio::table]] account
{
    asset balance;

    uint64_t primary_key() const { return balance.symbol.code().raw(); }
};

typedef eosio::multi_index<"accounts"_n, account> accounts_index;


struct [[eosio::table]] market_dfs {
    uint64_t    mid;
    name contract0;
    name contract1;
    symbol sym0;
    symbol sym1;
    asset reserve0;
    asset reserve1;
    uint64_t liquidity_token;
    double price0_last;
    double price1_last;        
    uint64_t price0_cumulative_last;     
    uint64_t price1_cumulative_last;        
    time_point_sec last_update;         
    auto primary_key() const { return mid; }
};
typedef eosio::multi_index<"markets"_n, market_dfs> dfs_markets;


struct [[eosio::table]] liqs2_dfs {
    uint64_t    mid;
    uint64_t token;
    asset bal0;
    asset bal1;
    time_point_sec start;    
    auto primary_key() const { return mid; }
};
typedef eosio::multi_index<"liqs2"_n, liqs2_dfs> dfs_liqs2;



struct token {
    name contract;
    symbol symbol;

    std::string to_string() const {
        return contract.to_string() + "-" + symbol.code().to_string();
    };
};

struct [[eosio::table]] pairs_box {
    uint64_t            id;
    token               token0;
    token               token1;
    asset               reserve0;
    asset               reserve1;
    uint64_t            liquidity_token;
    double              price0_last;
    double              price1_last;
    double              price0_cumulative_last;
    double              price1_cumulative_last;
    time_point_sec      block_time_last;

    uint64_t primary_key() const { return id; }
};
typedef eosio::multi_index< "pairs"_n, pairs_box > box_pairs;


struct [[eosio::table]] user_info_box {
    name                   owner;
    uint64_t               debt;
    uint64_t               liquidity;

    uint64_t primary_key() const { return owner.value; }
};
typedef eosio::multi_index< "userinfo"_n, user_info_box > box_user_info;


struct [[eosio::table]]  pair_ob {
    uint64_t pair_id;
    name contract0;
    name contract1;
    symbol symbol0;
    symbol symbol1;
    asset reserve0;
    asset reserve1;
    uint64_t liquidity_token = 0;
    double price0_last = 0.0;
    double price1_last = 0.0;
    time_point_sec update_time;

    uint64_t primary_key() const { return pair_id; }      
};
typedef eosio::multi_index< "pair"_n, pair_ob > ob_pair;


struct [[eosio::table]] liquidity_ob {
      uint64_t pair_id;
      uint64_t token;
      asset balance0;
      asset balance1;
      time_point_sec create_time;
      time_point_sec unlock_time;

      uint64_t primary_key() const { return pair_id; }
};
typedef eosio::multi_index< "liquidity"_n, liquidity_ob > ob_liquidity;


struct [[eosio::table]] members_pink
{
        name        owner;
        uint8_t     grade;
        name        recommender;
        uint64_t    exptime;         
        uint64_t    acttime; 

        uint64_t primary_key() const { return owner.value; }
};
typedef multi_index<name("members"), members_pink> pink_members;   
