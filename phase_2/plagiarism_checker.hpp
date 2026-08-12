#include "structures.hpp"
#include <vector>
#include <queue>
#include <utility>
#include <chrono>
#include <mutex>
#include<shared_mutex>
#include <unordered_map>
#include <unordered_set>
#include <thread>
#include <algorithm>
#include <string>
#include <condition_variable>
using namespace std;
using namespace chrono;

class plagiarism_checker_t
{
public:
    plagiarism_checker_t(void);
    plagiarism_checker_t(vector<shared_ptr<submission_t>> __submissions);
    ~plagiarism_checker_t(void);
    void add_submission(shared_ptr<submission_t> __submission);

private:
    bool terminate_flag = false;

    queue<shared_ptr<submission_t>> submission_queue;
    queue<shared_ptr<submission_t>> tokenize_queue;
    thread tokenize_thread;
    thread submission_thread;

    mutex submission_queue_mutex;
    mutex tokenize_queue_mutex;
    shared_mutex map_tokens_mutex;
    shared_mutex map_timestamp_mutex;
    shared_mutex hash_map_mutex1;
    shared_mutex hash_map_mutex2;
    mutex found_mutex;

    condition_variable tokenize_cv;
    condition_variable submission_cv;

    unordered_map<size_t, vector<pair<shared_ptr<submission_t>, int>>> hash_map1;
    unordered_map<size_t, vector<pair<shared_ptr<submission_t>, int>>> hash_map2;
    unordered_map<shared_ptr<submission_t>, vector<int>> map_tokens;
    unordered_map<shared_ptr<submission_t>, time_point<steady_clock>> map_timestamp;
    unordered_set<shared_ptr<submission_t>> found;
    unordered_set<shared_ptr<submission_t>> initial_subs;

    void process_submission();
    void process_tokenization();

    void insert_tokens(shared_ptr<submission_t> submission, vector<int> &tokens);
    bool get_tokens(shared_ptr<submission_t> submission, vector<int> &tokens);

    void insert_timestamp(shared_ptr<submission_t> submission, time_point<steady_clock> &timestamp);
    bool get_timestamp(shared_ptr<submission_t> submission, time_point<steady_clock> &timestamp);

    void insert_hash1(size_t hash, shared_ptr<submission_t> submission, int index);
    bool get_hash1(size_t hash, vector<pair<shared_ptr<submission_t>, int>> &sub_pairs);

    void insert_hash2(size_t hash, shared_ptr<submission_t> submission, int index);
    bool get_hash2(size_t hash, vector<pair<shared_ptr<submission_t>, int>> &sub_pairs);

    void compute_rolling_hash(shared_ptr<submission_t> submission, int length);
    bool check_tokens(shared_ptr<submission_t> s1, shared_ptr<submission_t> s2, int i1, int i2, vector<bool> &visited, int length);
    bool check_tokens(shared_ptr<submission_t> s1, shared_ptr<submission_t> s2, int i1, int i2, int length);

    void flag_sub(shared_ptr<submission_t> submission);
    void check_for_plag_1_and_2(shared_ptr<submission_t> submission);
    void check_for_plag_3(shared_ptr<submission_t> submission);
    void check_for_plag(shared_ptr<submission_t> submission);
};
