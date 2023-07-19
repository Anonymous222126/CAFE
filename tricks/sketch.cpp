#include <bits/stdc++.h>

using namespace std;


vector<uint32_t> loaddata(const char *filename = "../criteo/kaggle_processed_sparse.bin") {
  printf("Open %s \n", filename);
  FILE *pf = fopen(filename, "rb");
  if (!pf) {
    printf("%s not found!\n", filename);
    exit(-1);
  }

  vector<uint32_t> vec;
  double ftime = -1;
  char trace[30];
  while (fread(trace, 1, 21, pf)) {
    uint32_t tkey = *(uint32_t *)(trace);
    vec.push_back(tkey);
  }
  fclose(pf);
  return vec;
}

extern "C" {
    int ins[16384], que[16384];
    
    class SS {
    private:
        int n, k, s, lim, num;
        int Threshold;
        double tot;
        double p, x, y;
        struct Bucket{
            uint32_t val[4];
            float cnt[4];
            int dic[4];
            double sum;
        }*bucket;
        uint32_t Hash(uint32_t val) {
            return (val % s + s) % s;
        }
    public:
        vector<uint32_t> Top_K;
        queue<uint32_t> hot_id;
        
        SS(int k = 200, int lim = 130670): k(k), lim(lim) {
            Threshold = k;
            s = lim;
            tot = 0;
            num = 0;
            printf("size: %d\n", s);
            bucket = new Bucket [s];
            n = 0;
            x = 0.25;
            y = 0.25;
            for (int i = 1; i < lim; ++i)
                hot_id.push(i);
            for (int i = 0; i < s; ++i) {
                bucket[i].sum = 0;
                memset(bucket[i].cnt, 0, sizeof(bucket[i].cnt));
                memset(bucket[i].dic, 0, sizeof(bucket[i].dic));
            }
        }
        int query(uint32_t val) {
            int key = Hash(val);
            for (int i = 0; i < 4; ++i) {
                if (bucket[key].cnt[i] != 0 && bucket[key].val[i] == val) {
                    if (bucket[key].dic[i]) return -bucket[key].dic[i];
                }
            }
            return val;
        }
        void check(uint32_t val) {
            int key = Hash(val);
            for (int i = 0; i < 4; ++i) {
                if (bucket[key].cnt[i] && bucket[key].dic[i]) {
                    if (bucket[key].cnt[i] < bucket[key].sum * y) {
                        hot_id.push(bucket[key].dic[i]);
                        bucket[key].dic[i] = 0;
                    }
                }
            }
        }
        void decay() {
            printf("decay: hot_nums: %d, tot: %lf %lld\n", num, tot, 1ll * s * k);
            for (int key = 0; key < s; ++key) {
                for (int i = 0; i < 4; ++i) {
                    if (bucket[key].dic[i] && bucket[key].cnt[i] * 0.99 < k){
                        hot_id.push(bucket[key].dic[i]);
                        bucket[key].dic[i] = 0;
                    }
                    bucket[key].cnt[i] *= 0.99;
                }
            }
            tot = 0;
        }
        int Insert(uint32_t val, float v) {
            tot += v;
            int key = Hash(val), id = 0;
            for (int i = 0; i < 4; ++i) {
                if (bucket[key].cnt[i] && bucket[key].val[i] == val) {
                    bucket[key].cnt[i] += v;
                    bucket[key].sum += v;
                    if (bucket[key].cnt[i] >= k && !hot_id.empty() && !bucket[key].dic[i]) {
                        bucket[key].dic[i] = hot_id.front(), Top_K.push_back(val), id = 1;
                        hot_id.pop();
                        //printf("%d %d %d %ld\n", key, val, bucket[key].dic[i], hot_id.size());
                        ++num;
                        if (num % 10000 == 0) {
                            printf("num: %d\n", num);
                        }
                    }
                    while(i && bucket[key].cnt[i] > bucket[key].cnt[i-1]) {
                        swap(bucket[key].cnt[i], bucket[key].cnt[i-1]);
                        swap(bucket[key].val[i], bucket[key].val[i-1]);
                        swap(bucket[key].dic[i], bucket[key].dic[i-1]);
                        --i;
                    }
                    return id;
                }
            }
            for (int i = 0; i < 4; ++i) {
                if (bucket[key].cnt[i] == 0) {
                    bucket[key].sum += v;
                    bucket[key].cnt[i] = v;
                    bucket[key].val[i] = val;
                    return 0;
                }
            }
            if (!bucket[key].dic[3]) {
                bucket[key].cnt[3] += v;
                bucket[key].val[3] = val;
                bucket[key].sum += v;
            }
            return 0;
        }
        int* batch_query(uint32_t *data, int len) {
            for (int i = 0; i < len; ++i) {
                que[i] = query(data[i]);
            }
            // for (int i = 0; i < len; ++i)
            //    check(data[i]);
            return que;
        }
        int* batch_insert(uint32_t *data, int len) {
            //printf("%d\n", len);
            if (tot > 1ll * s * k * 10) decay();
            for (int i = 0; i < len; ++i) {
                //printf("%d %d\n", i, data[i]);
                ins[i] = Insert(data[i], 1);
            }
            return ins;
        }
        int* batch_insert_val(uint32_t *data, float *v, int len) {
            //printf("%d\n", len);
            if (tot > 1ll * s * k * 10) decay();
            for (int i = 0; i < len; ++i) {
                //printf("%d %d\n", i, data[i]);
                ins[i] = Insert(data[i], v[i]);
            }
            return ins;
        }
        vector<uint32_t> ans(){
            sort(Top_K.begin(), Top_K.end());
            Top_K.resize(unique(Top_K.begin(), Top_K.end()) - Top_K.begin());
            return Top_K;
        } 
    }ss;
    /*
    void Analyse(vector<uint32_t> ans, vector<uint32_t> pred) {
        sort(ans.begin(), ans.end());
        sort(pred.begin(), pred.end());
        pred.resize(unique(pred.begin(), pred.end()) - pred.begin());
        ans.resize(unique(ans.begin(), ans.end()) - ans.begin());
        int ptr = 0, acc = 0;
        for (auto i : ans) {
            while(ptr < pred.size() && pred[ptr] < i) ++ptr;
            if (pred[ptr] == i) ++ acc;
        }
        float Acc = 1.0 * acc / pred.size();
        float Rec = 1.0 * acc / ans.size();
        printf("answer: %ld  pred: %ld\n", ans.size(), pred.size());
        printf("Accuracy: %lf  Recall: %lf \n", 
        Acc, Rec);
        if (Rec < 0.95) exit(0);
    }/*
    void showsketch(uint32_t *data, int sz) {
        cout << "sz = " << sz << endl;
        unordered_map<uint32_t, int> mp;
        Sketch Hash_table = Sketch(sz, Threshold);
        vector<uint32_t> ans;
        for (int i = 0; i < sz; ++i) {
            Hash_table.Insert(data[i]);
            mp[data[i]] ++;
            if (mp[data[i]] == Threshold) ans.push_back(data[i]);
        }
        Analyse(ans, Hash_table.Top_K);
    }

    vector<uint32_t> runsketch(uint32_t *data, int sz) {
        cout << "data = " << sz << endl;
        Sketch Hash_table = Sketch(sz, Threshold);
        vector<uint32_t> ans;
        for (int i = 0; i < sz; ++i)
            Hash_table.Insert(data[i]);
        return Hash_table.ans();
    }*/
    float cntm[16384];
    class CUsketch{
    public:
        int k, n;
        float** cnt;
        int* Key;
        CUsketch(int k = 20, int n = 97):k(k), n(n){
            cnt = new float*[k];
            Key = new int[k];
            for (int i = 0; i < k; ++i){
                cnt[i] = new float[n];
                for (int j = 0; j < n; ++j) 
                    cnt[i][j] = 0;
            }
        }
        void insert(uint32_t key, float v) {
            uint32_t p = 998244353;
            int id, mn = 1e9;
            for (int i = 0; i < k; ++i) {
                key = (key + 1) * p;
                Key[i] = key % n;
                if (cnt[i][Key[i]] > mn) 
                    mn = cnt[i][Key[i]],
                    id = i;
                p = p * 998244353u;
            }
            cnt[id][Key[id]] += v;
        }
        float query(uint32_t key) {
            uint32_t p = 998244353;
            float mn = 1e9;
            for (int i = 0; i < k; ++i) {
                key = (key + 1) * p;
                Key[i] = key % n;
                if (cnt[i][key % n] > mn) 
                    mn = cnt[i][Key[i]],
                p = p * 998244353u;
            }
            return mn;
        }
        void batch_insert(uint32_t *data, int len) {
            for (int i = 0; i < len; ++i)
                insert(data[i], 1);
        }
        void batch_insert_val(uint32_t *data, float* v, int len) {
            for (int i = 0; i < len; ++i)
                insert(data[i], v[i]);
        }
        float* batch_cnt(uint32_t *data, int len) {
            for (int i = 0; i < len; ++i)
                cntm[i] = query(data[i]);
            return cntm;
        }
    }CU;
    
    int* batch_query(uint32_t *data, int len) {
        return ss.batch_query(data, len);
    }
    float* batch_cnt(uint32_t *data, int len) {
        return CU.batch_cnt(data, len);
    }
    int* batch_insert(uint32_t *data, int len) {
        return ss.batch_insert(data, len);
    }
    int* batch_insert_val(uint32_t *data, float *v, int len) {
        return ss.batch_insert_val(data, v, len);
    }
    void init(int n, int Threshold){
        ss = SS(Threshold, n);
    }
}


int main() {
    return 0;
}