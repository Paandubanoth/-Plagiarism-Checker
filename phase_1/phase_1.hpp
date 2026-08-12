#include <bits/stdc++.h>
using namespace std;
class match
{
private:
    int start1;
    int start2;
    int end1;
    int end2;
    int length;

public:
    match(int s1, int s2, int e1, int e2, int l)
    {
        start1 = s1;
        start2 = s2;
        end1 = e1;
        end2 = e2;
        length = l;
    }
    int get_start1() const { return start1; };
    int get_start2() const { return start2; };
    int get_end1() const { return end1; };
    int get_end2() const { return end2; };
    int get_length() const { return length; };
};
class segtree
{
public:
    vector<int> tree;
    int leaf_count;
    segtree(int sz)
    {
        if (sz <= 0 || sz > 1e6)
        {
            leaf_count = 1;
            tree.resize(1, 0);
            return;
        }
        leaf_count = 1;
        while (leaf_count < sz)
            leaf_count <<= 1;
        tree.resize(2 * leaf_count - 1, 0);
    }
    void update(int ind, int val)
    {
        ind += leaf_count - 1;
        tree[ind] = max(val, tree[ind]);
        while (ind)
        {
            ind = (ind - 1) / 2;
            tree[ind] = max(tree[2 * ind + 1] , tree[2 * ind + 2]);
        }
    }
    int query(int l, int r, int l1, int r1, int ind)
    {
        if (l1 >= r || r1 <= l)
            return 0;
        if (l <= l1 && r >= r1)
            return tree[ind];
        int mid = (l1 + r1) / 2;
        return max(query(l, r, l1, mid, 2 * ind + 1), query(l, r, mid, r1, 2 * ind + 2));
    }
    int query(int l, int r)
    {
        return query(l, r, 0, leaf_count, 0);
    }
};
class segtree2
{
public:
    vector<segtree> trees;
    int leaf_count;
    segtree2(int size1, int size2)
    {
        if (size1 <= 0 || size2 <= 0 || size1 > 1e6 || size2 > 1e6)
        {
            leaf_count = 1;
            trees.resize(1, segtree(1));
            return;
        }
        leaf_count = 1;
        while (leaf_count < size1)
            leaf_count <<= 1;
        trees.resize(2 * leaf_count - 1, segtree(size2));
    }
    void update(int x, int y, int val)
    {
        x += leaf_count - 1;
        trees[x].update(y, val);
        while (x)
        {
            x = (x - 1) / 2;
            trees[x].update(y, val);
        }
    }
    int query(int x1,int x2,int y1,int y2,int t1,int b1, int ind)
    {
        if (t1 >= x2 || b1 <= x1)
            return 0;
        if (x1 <= t1 && x2 >= b1)
            return trees[ind].query(y1, y2);
        int mid = (t1 + b1) / 2;
        return max(query(x1,x2,y1,y2,t1,mid,2*ind+1) , query(x1,x2,y1,y2,mid,b1, 2 * ind + 2));
    }
    int query(int x1, int x2, int y1, int y2)
    {
        return query(x1,x2,y1,y2, 0, leaf_count, 0);
    }
};
unordered_map<int, vector<int>> Rolling_hash(vector<int> &s, int length)
{
    unordered_map<int, vector<int>> string_hash;
    int n = s.size();
    size_t q = 1e9 + 7;
    size_t mul = 10;
    size_t h = 1;
    size_t hash = 0;
    for (size_t i = 0; i < length; i++)
    {
        hash = (hash * mul + s[i]) % q;
        if (i < length - 1)
            h = (h * mul) % q;
    }
    for (size_t i = 0; i <= n - length; i++)
    {
        string_hash[hash].push_back(i);
        if (i + length < n)
        {
            hash = (hash + q - (s[i] * h) % q) % q;
            hash = (hash * mul + s[i+length]) % q;
        }
    }
    return string_hash;

}
int sum_of_exact_matches(vector<int> &s1, vector<int> &s2)
{
    if (s1.size() < 10 || s2.size() < 10)
        return 0;
    unordered_map<int, vector<int>> hash1 = Rolling_hash(s1, 10);
    vector<match > matches;
    int n = s2.size();
    size_t q = 1e9 + 7;
    size_t mul = 10;
    size_t h = 1;
    size_t hash = 0;
    for (int j = 0; j < 10; j++)
    {
        hash = (hash * mul + s2[j]) % q;
        if (j < 9)
            h = (h * mul) % q;
    }
    for (int i = 0; i <= n - 10; i++)
    {
        if (hash1.find(hash) != hash1.end())
        {
            for (int k : hash1[hash])
            {
                int i1 = k;
                int i2 = i;
                int len = 0;
                while (len + i1 < s1.size() && len + i2 < s2.size() && s1[i1 + len] == s2[i2 + len])
                {
                    len++;
                    if (len >= 10 && len <= 20)
                    {
                        matches.push_back(match(i1, i2, i1 + len - 1, i2 + len - 1, len));
                    }
                }
            }
        }
        if (i + 10 < n)
        {
            hash = (hash + q - (s2[i] * h) % q) % q;
            hash = (hash * mul + s2[i+10]) % q;
        }
    }
    sort(matches.begin(), matches.end(), [](const match &a, const match &b) {
        if (a.get_end1() == b.get_end1()) return a.get_end2() < b.get_end2();
        return a.get_end1() < b.get_end1();
    });

    int m = matches.size();
    // vector<int> dp(m, 0);
    // int max_total = 0;
    
    // for (int i = 0; i < m; i++) {
    //     dp[i] = matches[i].get_length();
    //     for (int j = 0; j < i; j++) {
    //         if (matches[j].get_end1() < matches[i].get_start1() &&
    //             matches[j].get_end2() < matches[i].get_start2()) {
    //             dp[i] = max(dp[i], dp[j] + matches[i].get_length());
    //         }
    //     }
    //     max_total = max(max_total, dp[i]);
    // }
    // return max_total;
    segtree2 stree(s1.size(), s2.size());
    int max_total = 0;
    vector<int> ends1, ends2;
    for(int i=0;i<m;i++){
        ends1.push_back(matches[i].get_end1());
        ends2.push_back(matches[i].get_end2());
    }
    sort(ends1.begin(), ends1.end());
    sort(ends2.begin(), ends2.end());
    ends1.erase(unique(ends1.begin(), ends1.end()), ends1.end());
    ends2.erase(unique(ends2.begin(), ends2.end()), ends2.end());
    auto compress=[](int x, vector<int> &v) {
        return lower_bound(v.begin(), v.end(), x) - v.begin();
    };
    for(int i=0;i<m;i++){
        int val=stree.query(0, compress(matches[i].get_start1(), ends1), 
                            0, compress(matches[i].get_start2(), ends2));
        val += matches[i].get_length();
        stree.update(compress(matches[i].get_end1(), ends1), 
                     compress(matches[i].get_end2(), ends2), val);
    }
    return stree.query(0, ends1.size(), 0, ends2.size());
}
tuple<int, int, int> find_max_approx_match(vector<int> &s1, vector<int> &s2, double x = 0.8)
{
    int n1 = s1.size();
    int n2 = s2.size();
    if (n1 < 30 || n2 < 30)
        return {0, -1, -1};
    vector<vector<int>> dp(n1 + 1, vector<int>(n2 + 1, 0));
    vector<vector<int>> mismatches(n1 + 1, vector<int>(n2 + 1, 0));
    int max_length = 0;
    int st1 = -1, st2 = -1;
    for (int i = 1; i <= n1; i++)
    {
        for (int j = 1; j <= n2; j++)
        {
            if (s1[i - 1] == s2[j - 1])
                mismatches[i][j] = mismatches[i - 1][j - 1];
            else
                mismatches[i][j] = mismatches[i - 1][j - 1] + 1;
            dp[i][j] = dp[i - 1][j - 1] + 1;
            if (mismatches[i][j] > (1 - x) * dp[i][j])
            {
                mismatches[i][j] = 0;
                dp[i][j] = 0;
            }
            if (dp[i][j] > max_length && dp[i][j] >= 30)
            {
                max_length = dp[i][j];
                st1 = i - max_length;
                st2 = j - max_length;
            }
        }
    }
    return make_tuple(max_length, st1, st2);
}
array<int, 5> match_submissions(vector<int> &submission1, vector<int> &submission2)
{
    array<int, 5> result = {0, 0, 0, 0, 0};
    result[1] = sum_of_exact_matches(submission1, submission2);
    tuple<int, int, int> t = find_max_approx_match(submission1, submission2, 0.8);
    result[2] = get<0>(t);
    result[3] = get<1>(t);
    result[4] = get<2>(t);
    if (result[1] >= 0.3 * min(submission1.size(), submission2.size()))
    {
        result[0] = 1;
    }
    else
    {
        result[0] = 0;
    }
    return result;
}