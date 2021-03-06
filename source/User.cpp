#include "User.hpp"
#include "ReadInput.hpp"
#include "LSH.hpp"
#include "CosineSimilarity.hpp"
#include "utility.hpp"

#include <math.h> 

using namespace std;

User::User(){}

User::User(string &uid)
:userid(uid){}

User::User(const User& u)
:userid(u.userid),tweets(u.tweets),crypto_values(u.crypto_values),vector(u.vector){}

User::~User(){}

void User::addTweet(Tweet* tweet){
  tweets.push_back(tweet);
}

void User::print(){
  cout << userid << ":{";
  for(auto it=tweets.begin(); it!=tweets.end(); it++){
    cout << (*it)->getTweetId() << ",";
  }
  cout << "}\n";
}

std::vector<Tweet*> User::getTweets(){
  return tweets;
}

std::string User::getUserId(){
  return userid;
}

myvector User::getVector() const{
  return vector;
}

extern vector<string> CryptoOrder;

std::vector<std::string> User::getTopCryptos(int n){
  std::vector<pair<int,double>> myheap; //max heap
  double max = vector[0];

  for(int i=0; i<vector.size(); i++){
    if(myheap.size() < n){
      //not filled yet, just insert and remember the lowest max
      myheap.push_back(pair<int,double>(i,vector[i]));
      push_heap(myheap.begin(), myheap.end(), [](pair<int,double>&a,
                pair<int,double>&b){return a.second<b.second;});
      if(vector[i] < max)
        max=vector[i];
    }
    else{
      if(vector[i] >= max){
        pop_heap(myheap.begin(),myheap.end());  //reorder elements
        myheap.pop_back();                   //remove max dist element
        myheap.push_back(pair<int,double>(i,vector[i]));
        push_heap(myheap.begin(), myheap.end(), [](pair<int,double>&a,
                  pair<int,double>&b){return a.second<b.second;});
        max = myheap.front().second;;
      }
    }
  }
  std::vector<string> result;
  for(int i=0; i<myheap.size(); i++){
    if(result.size() >= n)
      break;  //got enough results, stop
    //take max element from heap
    int pos = myheap.front().first;
    pop_heap(myheap.begin(),myheap.end());  //delete max element
    myheap.pop_back();
    string crypto_name = CryptoOrder[pos];
    if(crypto_values.find(crypto_name)==crypto_values.end()){
      //only add unkown cryptos to result
      result.push_back(crypto_name);
    }
  }
  return result;

}


vector<User*> GroupTweetsByUser(UserMap &usermap, std::vector<Tweet*> &Tweets){
  vector<User*> users;
  for(auto it=Tweets.begin(); it!=Tweets.end(); it++){
    string uid = (*it)->getUserId();
    if(usermap.find(uid) == usermap.end()){
      //user not in map
      User* user = new User(uid); //create user
      users.push_back(user);
      usermap[uid] = user;
    }
    usermap[uid]->addTweet(*it);
  }
  cout << "Found " << users.size() << " different users." << endl;
  return users;
}

/*take the score of every user's tweet and add it to the
cryptocurrency/ies the tweet mentions*/
void User::CalcCryptoValues(map<string,string> &CryptoNameMap){
  for(auto tweet=tweets.begin(); tweet!=tweets.end(); tweet++){

    std::vector<std::string> tokens = (*tweet)->getTokens();
    for(auto token=tokens.begin(); token!=tokens.end(); token++){

      map<string,string>::iterator element;
      if( (element=CryptoNameMap.find(*token)) != CryptoNameMap.end()){
        //token is a cryptocurrency
        string crypto = element->second;

        map<string,double>::iterator val;
        if((val=crypto_values.find(crypto)) == crypto_values.end()){
          //this crypto was previously unknown to the user
          crypto_values[crypto] = (*tweet)->getScore();
        }
        else{
          //this crypto is known to the user and already has a value
          crypto_values[crypto] += (*tweet)->getScore();
        }
      }
    }
  }
}

double User::AverageValue(){
  double sum=0;
  int num_cryptos=0;
  for(auto it=crypto_values.begin(); it!=crypto_values.end(); it++){
    sum += it->second;
    num_cryptos++;
  }
  return sum/num_cryptos;
}

/*Create myvector using crypto_values,ordered as in CryptoSet.
If there are unknown crypto_values use average value.
Values are normalised by subtracting average_value from each of them.*/
myvector* User::Vectorize(set<string> CryptoSet){
  double average_value = AverageValue();
  if(isnan(average_value))  //if zero vector
    return NULL;
  std::vector<double> myvalues(CryptoSet.size());
  int i=0;
  for(auto it=CryptoSet.begin(); it!=CryptoSet.end(); it++,i++){
    map<string,double>::iterator val;
    if((val=crypto_values.find(*it)) == crypto_values.end()){
      //crypto unknown to user
      myvalues[i] = average_value;
    }
    else{
      //crypto known to user and has a value
      myvalues[i] = val->second-average_value;
    }
  }
  vector = myvector(myvalues,userid);
  return &vector;
}

//rate unkown cryptos by neraest neighbor crypto_scores and their similarity
void User::RateByNNSimilarity(std::vector<HashTable*> &LSH_Hashtables,
  set<string> &CryptoSet){
  //find nearest neighbors of this user
  std::vector<myvector*> nns = NearestNeighbors(LSH_Hashtables,vector,CmdArgs::NUM_NN);
  //calculate similarities to nns
  std::vector<double> similarities(nns.size());
  double sim_sum=0;
  int i=0;
  for(auto it=nns.begin(); it!=nns.end(); it++,i++){
    similarities[i] = CosineVectorSimilarity(vector.begin(),vector.end(),(*it)->begin());
    sim_sum += similarities[i];
  }
  //approximate unknown crypto values using similarities
  int coord=0;
  for(auto crypto=CryptoSet.begin(); crypto!=CryptoSet.end(); crypto++,coord++){
    //rate only unknown crypto values
    if(crypto_values.find(*crypto) == crypto_values.end())
      continue;
    double nn_ratings=0;
    for(int i=0; i<nns.size(); i++)
      nn_ratings += (similarities[i]*((*nns[i])[coord]));
    if(sim_sum != 0)
      vector[coord] = nn_ratings/sim_sum;
  }
}

void User::RateByClusterSimilarity(const Cluster& Cluster,set<string>&CryptoSet){
  //find nearest neighbors of this user
  std::vector<myvector*> nns = Cluster.getMembers();
  //calculate similarities to nns
  std::vector<double> similarities(nns.size());
  double sim_sum=0;
  int i=0;
  for(auto it=nns.begin(); it!=nns.end(); it++,i++){
    similarities[i] = CosineVectorSimilarity(vector.begin(),vector.end(),(*it)->begin());
    sim_sum += similarities[i];
  }
  //approximate unknown crypto values using similarities
  int coord=0;
  for(auto crypto=CryptoSet.begin(); crypto!=CryptoSet.end(); crypto++,coord++){
    //rate only unknown crypto values
    if(crypto_values.find(*crypto) == crypto_values.end())
      continue;
    double nn_ratings=0;
    for(int i=0; i<nns.size(); i++)
      nn_ratings += (similarities[i]*((*nns[i])[coord]));
    if(sim_sum != 0)
      vector[coord] = nn_ratings/sim_sum;
  }
}

vector<myvector*> VectorizeUsers(vector<User*> &Users, set<string> &CryptoSet){
  vector<myvector*> UserVectors;
  for(auto it=Users.begin(); it!=Users.end(); it++){
    myvector* vector = (*it)->Vectorize(CryptoSet);
    if(vector==NULL){
      //remove users that didn't mention any crypto
      delete(*it);
      *it = NULL;
    }
    else if(isZeroVector(vector->begin(),vector->end())){
      //remove users that didn't mention any crypto
      delete(*it);
      *it = NULL;
    }
    else{
      UserVectors.push_back(vector);
    }
  }
  return UserVectors;
}

//create virutal users and assign them the tweets from clusters
vector<User*> CreateVirtualUsers(const std::vector<Cluster> &Clusters,
  map<string,Tweet*>&tweet_id_map, map<string,string> &CryptoNameMap){
  vector<User*> vusers(Clusters.size());
  for(int i=0; i<Clusters.size(); i++){
    vusers[i] = new User;
    vector<myvector*> cluster_members = Clusters[i].getMembers();
    for(auto it=cluster_members.begin(); it!=cluster_members.end(); it++){
      if(tweet_id_map.find((*it)->get_id()) == tweet_id_map.end())
        continue; //exit(TWEET_NOT_IN_MAP);
      vusers[i]->addTweet(tweet_id_map[(*it)->get_id()]);
    }
    vusers[i]->CalcCryptoValues(CryptoNameMap);
  }
  return vusers;
}

vector<User*> CopyUsers(vector<User*> &Users){
  vector<User*> CopiedUsers(Users.size());
  for(int i=0; i<Users.size(); i++){
    if(Users[i] == NULL)
      CopiedUsers[i] = NULL;
    else
      CopiedUsers[i] = new User(*(Users[i]));
  }
  return CopiedUsers;
}

vector<myvector> UserToMyvector(vector<User*> Users){
  vector<myvector> result;
  for(auto it=Users.begin(); it!=Users.end(); it++){
    if(*it == NULL) continue;
    result.push_back((*it)->getVector());
  }
  return result;
}

/*From myvector* memebers of a cluster extact the userid
(which is set to be the id of the vector), and usering the usermap and userid as
the key find the user pointer. Mirroring the cluster of myvectors to User.*/
vector<vector<User*>> UserCluster(const vector<Cluster>& Clusters, UserMap& usermap){
  vector<vector<User*>> result(Clusters.size());
  for(int i=0; i<Clusters.size(); i++){
    vector<myvector*> members = Clusters[i].getMembers();
    vector<User*> users(members.size());
    for(int j=0; j<members.size(); j++){
      users[j] = usermap[members[j]->get_id()];
    }
    result.push_back(users);
  }
  return result;
}

UserMap MapUsersByID(vector<User*>& Users){
  UserMap map;
  for(auto it=Users.begin(); it!=Users.end(); it++){
    if(*it == NULL) continue;
    map[(*it)->getUserId()] = *it;
  }
  return map;
}
