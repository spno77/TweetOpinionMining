#ifndef USER_HPP
#define USER_HPP

#include "Tweet.hpp"
#include "myvector.hpp"

#include <vector>
#include <string>
#include <map>
#include <set>

class User{
    std::string userid;
    std::vector<Tweet*> tweets;
    std::map<std::string,double> crypto_values;
  public:
    User();
    User(std::string &uid);
    ~User();
    void addTweet(Tweet* tweet);
    void print();
    std::vector<Tweet*> getTweets();
    std::string getUserId();
    /*Take the score of every user's tweet and add it to the
    cryptocurrency/ies the tweet mentions*/
    void CalcCryptoValues(std::map<std::string,std::string> &CryptoNameMap);
    //Average value(sentiment) of user towards cryptocurrencies he mentions
    double AverageValue();
    /*Create myvector using crypto_values,ordered as in CryptoSet.
    If there are unknown crypto_values use average value.*/
    myvector Vectorize(std::set<std::string> CryptoSet);
};

typedef std::map<std::string,User*> UserMap; //maps users by userid

/*Assign each tweet to its user and return all users (creates Users).
Ignore those that dont mention any crypto currencies.*/
std::vector<User*> GroupTweetsByUser(UserMap &usermap, std::vector<Tweet*> &);

#endif