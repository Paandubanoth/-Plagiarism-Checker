#include "plagiarism_checker.hpp"
using namespace std;
using namespace std::chrono;

void plagiarism_checker_t::insert_timestamp(shared_ptr<submission_t> submission, time_point<steady_clock> &timestamp)
{
    unique_lock lock(map_timestamp_mutex);
    map_timestamp[submission] = timestamp;
}

bool plagiarism_checker_t::get_timestamp(shared_ptr<submission_t> submission, time_point<steady_clock> &timestamp)
{
    shared_lock lock(map_timestamp_mutex);
    auto itr = map_timestamp.find(submission);
    if (itr == map_timestamp.end())
    {
        return false;
    }
    timestamp = itr->second;
    return true;
}

void plagiarism_checker_t::insert_tokens(shared_ptr<submission_t> submission, vector<int> &tokens)
{
    unique_lock lock(map_tokens_mutex);
    map_tokens[submission] = tokens;
}

bool plagiarism_checker_t::get_tokens(shared_ptr<submission_t> submission, vector<int> &tokens)
{
    shared_lock lock(map_tokens_mutex);
    auto itr = map_tokens.find(submission);
    if (itr == map_tokens.end())
    {
        return false;
    }
    tokens = itr->second;
    return true;
}

void plagiarism_checker_t::insert_hash1(size_t hash, shared_ptr<submission_t> submission, int ind)
{
    unique_lock lock(hash_map_mutex1);
    hash_map1[hash].push_back({submission, ind});
}

bool plagiarism_checker_t::get_hash1(size_t hash, vector<pair<shared_ptr<submission_t>, int>> &sub_pairs)
{
    shared_lock lock(hash_map_mutex1);
    auto itr = hash_map1.find(hash);
    if (itr == hash_map1.end())
    {
        return false;
    }
    sub_pairs = itr->second;
    return true;
}

void plagiarism_checker_t::insert_hash2(size_t hash, shared_ptr<submission_t> submission, int ind)
{
    unique_lock lock(hash_map_mutex2);
    hash_map2[hash].push_back({submission, ind});
}

bool plagiarism_checker_t::get_hash2(size_t hash, vector<pair<shared_ptr<submission_t>, int>> &sub_pairs)
{
    shared_lock lock(hash_map_mutex2);
    auto itr = hash_map2.find(hash);
    if (itr == hash_map2.end())
    {
        return false;
    }
    sub_pairs = itr->second;
    return true;
}

plagiarism_checker_t::plagiarism_checker_t()
{
    terminate_flag = false;
    tokenize_thread = thread(&plagiarism_checker_t::process_tokenization, this);
    submission_thread = thread(&plagiarism_checker_t::process_submission, this);
}

plagiarism_checker_t::plagiarism_checker_t(vector<shared_ptr<submission_t>> __submissions)
{
    terminate_flag = false;
    unique_lock lock(tokenize_queue_mutex);
    for (auto &submission : __submissions)
    {
        if (submission)
        {
            auto zero_time = steady_clock::now();
            insert_timestamp(submission, zero_time);
            initial_subs.insert(submission);
            tokenize_queue.push(submission);
            tokenize_cv.notify_one();
        }
    }
    tokenize_thread = thread(&plagiarism_checker_t::process_tokenization, this);
    submission_thread = thread(&plagiarism_checker_t::process_submission, this);
}

void plagiarism_checker_t::add_submission(shared_ptr<submission_t> __submission)
{
    unique_lock lock(tokenize_queue_mutex);
    if (__submission)
    {
        auto zero_time = steady_clock::now();
        insert_timestamp(__submission, zero_time);
        tokenize_queue.push(__submission);
        tokenize_cv.notify_one();
    }
}

plagiarism_checker_t::~plagiarism_checker_t()
{
    while (true)
    {
        scoped_lock lock(submission_queue_mutex, tokenize_queue_mutex);
        if (submission_queue.empty() && tokenize_queue.empty())
        {
            break;
        }
        this_thread::sleep_for(seconds(1));
    }
    terminate_flag = true;
    scoped_lock(tokenize_queue_mutex, submission_queue_mutex);
    tokenize_cv.notify_all();
    submission_cv.notify_all();
    if (tokenize_thread.joinable())
    {
        tokenize_thread.join();
    }
    if (submission_thread.joinable())
    {
        submission_thread.join();
    }
    unique_lock lock1(map_timestamp_mutex);
    map_timestamp.clear();
    unique_lock lock2(map_tokens_mutex);
    map_tokens.clear();
    unique_lock lock3(hash_map_mutex1);
    hash_map1.clear();
    unique_lock lock4(hash_map_mutex2);
    hash_map2.clear();
}

void plagiarism_checker_t::process_tokenization()
{
    while (true)
    {
        unique_lock lock(tokenize_queue_mutex);
        tokenize_cv.wait(lock, [this]()
                         { return !tokenize_queue.empty() || terminate_flag; });

        if (terminate_flag && tokenize_queue.empty())
        {
            break;
        }

        auto front = tokenize_queue.front();
        tokenize_queue.pop();

        tokenizer_t tokenizer(front->codefile);
        vector<int> tokens = tokenizer.get_tokens();
        insert_tokens(front, tokens);

        unique_lock submission_lock(submission_queue_mutex);
        submission_queue.push(front);
        submission_cv.notify_one();
    }
}

void plagiarism_checker_t::process_submission()
{
    while (true)
    {
        unique_lock lock(submission_queue_mutex);
        submission_cv.wait(lock, [this]()
                           { return !submission_queue.empty() || terminate_flag; });

        if (terminate_flag && submission_queue.empty())
            break;

        auto front = submission_queue.front();
        submission_queue.pop();

        compute_rolling_hash(front, 15);
        compute_rolling_hash(front, 75);
        check_for_plag(front);
    }
}

void plagiarism_checker_t::compute_rolling_hash(shared_ptr<submission_t> submission, int length)
{
    vector<int> tokens;
    if (!get_tokens(submission, tokens))
        return;
    if (tokens.size() < length)
        return;

    int n = tokens.size();
    size_t mod = 1e9 + 7;
    size_t base = 257;
    size_t hash = 0;
    size_t base_pow = 1;

    for (int i = 0; i < length; i++)
    {
        hash = (hash * base + tokens[i]) % mod;
        if (i < length - 1)
        {
            base_pow = (base_pow * base) % mod;
        }
    }

    for (int i = 0; i <= n - length; i++)
    {
        if (length == 15)
            insert_hash1(hash, submission, i);
        else if (length == 75)
            insert_hash2(hash, submission, i);

        if (i + length < n)
        {
            hash = (hash + mod - (tokens[i] * base_pow) % mod) % mod;
            hash = (hash * base + tokens[i + length]) % mod;
        }
    }
}

bool plagiarism_checker_t::check_tokens(shared_ptr<submission_t> s1, shared_ptr<submission_t> s2, int i1, int i2, vector<bool> &visited, int length)
{
    vector<int> tokens1, tokens2;
    get_tokens(s1, tokens1);
    get_tokens(s2, tokens2);
    for (int i = 0; i < length; i++)
    {
        if (i1 + i >= tokens1.size() || i2 + i >= tokens2.size() || tokens1[i1 + i] != tokens2[i2 + i] || visited[i1 + i])
        {
            return false;
        }
    }
    for (int i = 0; i < length; i++)
    {
        visited[i1 + i] = true;
    }
    return true;
}

bool plagiarism_checker_t::check_tokens(shared_ptr<submission_t> s1, shared_ptr<submission_t> s2, int i1, int i2, int length)
{
    vector<int> tokens1, tokens2;
    get_tokens(s1, tokens1);
    get_tokens(s2, tokens2);
    for (int i = 0; i < length; i++)
    {
        if (i1 + i >= tokens1.size() || i2 + i >= tokens2.size() || tokens1[i1 + i] != tokens2[i2 + i])
        {
            return false;
        }
    }
    return true;
}

void plagiarism_checker_t::flag_sub(shared_ptr<submission_t> submission)
{
    unique_lock lock(found_mutex);
    if (found.find(submission) != found.end() || initial_subs.find(submission) != initial_subs.end())
    {
        return;
    }
    if (submission->student)
    {
        submission->student->flag_student(submission);
    }
    if (submission->professor)
    {
        submission->professor->flag_professor(submission);
    }
    found.insert(submission);
}

void plagiarism_checker_t::check_for_plag_1_and_2(std::shared_ptr<submission_t> submission)
{
    vector<int> tokens;
    if (!get_tokens(submission, tokens))
        return;
    int length = 15;
    if (tokens.size() < length)
        return;
    int n = tokens.size();
    vector<bool> visited1(n,false);
    unordered_map<shared_ptr<submission_t>, vector<bool>> visited;
    unordered_map<shared_ptr<submission_t>, int> count;
    int counter = 0;

    time_point<steady_clock> sub_time, cur_time;
    get_timestamp(submission, sub_time);

    size_t mod = 1e9 + 7;
    size_t base = 257;
    size_t hash = 0;
    size_t base_pow = 1;

    for (int i = 0; i < length; i++)
    {
        hash = (hash * base + tokens[i]) % mod;
        if (i < length - 1)
        {
            base_pow = (base_pow * base) % mod;
        }
    }

    vector<pair<shared_ptr<submission_t>, int>> pairs;

    for (int i = 0; i <= n - length; i++)
    {
        pairs.clear();
        if (get_hash1(hash, pairs))
        {
            for (auto &itr : pairs)
            {
                auto cur = itr.first;
                int ind = itr.second;
                get_timestamp(cur, cur_time);
                if (visited.find(cur) == visited.end())
                {
                    visited[cur] = vector<bool>(n, false);
                }
                if (cur != submission && (cur_time<=sub_time) && check_tokens(submission, cur, i, ind, visited[cur], length))
                {
                    count[cur]++;
                }

                if(cur != submission  && (cur_time<=sub_time) && check_tokens(submission, cur, i, ind, visited1, length)){
                    counter++;
                }
            }
        }
        if (i + length < n)
        {
            hash = (hash + mod - (tokens[i] * base_pow) % mod) % mod;
            hash = (hash * base + tokens[i + length]) % mod;
        }
    }

    if (counter >= 20)
    {
        flag_sub(submission);
    }

    for (auto &itr : count)
    {
        if (itr.second >= 10)
        {
            auto cur = itr.first;
            get_timestamp(cur, cur_time);
            flag_sub(submission);
            if (sub_time-cur_time <= seconds(1))
            {
                flag_sub(cur);
            }
        }
    }
}
void plagiarism_checker_t::check_for_plag_3(std::shared_ptr<submission_t> submission)
{
    vector<int> tokens;
    if (!get_tokens(submission, tokens))
        return;
    int length = 75;
    if (tokens.size() < length)
        return;
    int n = tokens.size();

    unordered_set<shared_ptr<submission_t>> subs_75;
    time_point<steady_clock> sub_time, cur_time;
    get_timestamp(submission, sub_time);

    size_t mod = 1e9 + 7;
    size_t base = 257;
    size_t hash = 0;
    size_t base_pow = 1;

    for (int i = 0; i < length; i++)
    {
        hash = (hash * base + tokens[i]) % mod;
        if (i < length - 1)
        {
            base_pow = (base_pow * base) % mod;
        }
    }

    vector<pair<shared_ptr<submission_t>, int>> pairs;

    for (int i = 0; i <= n - length; i++)
    {
        pairs.clear();
        if (get_hash2(hash, pairs))
        {
            for (auto &itr : pairs)
            {
                auto cur = itr.first;
                int ind = itr.second;
                get_timestamp(cur, cur_time);
                if (cur != submission && (cur_time<=sub_time) && check_tokens(submission, cur, i, ind, 75))
                {
                    subs_75.insert(cur);
                }
            }
        }
        if (i + length < n)
        {
            hash = (hash + mod - (tokens[i] * base_pow) % mod) % mod;
            hash = (hash * base + tokens[i + length]) % mod;
        }
    }

    if (!subs_75.empty())
    {
        flag_sub(submission);
        for (auto &itr : subs_75)
        {
            get_timestamp(itr, cur_time);
            if (sub_time-cur_time <= seconds(1))
            {
                flag_sub(itr);
            }
        }
    }
}

void plagiarism_checker_t::check_for_plag(shared_ptr<submission_t> submission)
{
    check_for_plag_1_and_2(submission);
    check_for_plag_3(submission);
}