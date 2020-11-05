#include <bits/stdc++.h>
using namespace std;

int data[5];
map <string, int> m = {{"a", 0}, {"b", 1}, {"c", 2}, {"d", 3}, {"e", 4}};

int readers_count[5];
mutex l[5];
condition_variable reader_cond[5];
condition_variable writer_cond[5];
map <pair<int, string>, int> dic;

/*
	each transaction has a id,
	sequence of operations
	and the outcome
*/
class transaction {
public:
	int id;
	vector <string> ops;
	string outcome;
};

/*
	Helper function to aquire read lock of transaction whose id is "id" on "s"
*/
void aquire_read_lock(int id, string s) {
	mt19937 rng;
	rng.seed(random_device()());
	uniform_int_distribution<mt19937::result_type> dist(0, 20);
	unique_lock<mutex> lk(l[m[s]], defer_lock);
	this_thread::sleep_for(chrono::milliseconds(dist(rng)));
	lk.lock();
	/*
		wait until writer finishes off
	*/
	if (readers_count[m[s]] == -1) {
		reader_cond[m[s]].wait(lk, [s]() { return readers_count[m[s]] != -1; });
	}
	readers_count[m[s]]++;
	lk.unlock();
	dic[ {id, s}] = 1;
	cout << "R-lock [" << id << "," << s << "]\n";
}

/*
	Helper function to aquire write lock of transaction whose id is "id" on "s"
*/
void aquire_write_lock(int id, string s) {
	mt19937 rng;
	rng.seed(random_device()());
	uniform_int_distribution<mt19937::result_type> dist(0, 20);
	unique_lock<mutex> lk(l[m[s]], defer_lock);
	this_thread::sleep_for(chrono::milliseconds(dist(rng)));
	lk.lock();
	/*
		wait until the number of active readers are greater than 0
	*/
	if (readers_count[m[s]] > 0) {
		writer_cond[m[s]].wait(lk, [s]() { return readers_count[m[s]] == 0; });
	}
	readers_count[m[s]] = -1;
	lk.unlock();
	dic[ {id, s}] = 2;
	cout << "W-lock [" << id << "," << s << "]\n";
}

/*
	Helper function to release read/write lock of transaction whose id is "id" on "s"
*/
void release_lock(int id, string s) {
	if (dic[ {id, s}] == 1) {
		mt19937 rng;
		rng.seed(random_device()());
		uniform_int_distribution<mt19937::result_type> dist(0, 20);
		unique_lock<mutex> lk(l[m[s]], defer_lock);
		this_thread::sleep_for(chrono::milliseconds(dist(rng)));
		lk.lock();
		/*
			reduce the number of active readers
		*/
		readers_count[m[s]]--;
		if (readers_count[m[s]] == 0) {
			writer_cond[m[s]].notify_all();
		}
		lk.unlock();
	}
	else {
		mt19937 rng;
		rng.seed(random_device()());
		uniform_int_distribution<mt19937::result_type> dist(0, 20);
		unique_lock<mutex> lk(l[m[s]], defer_lock);
		this_thread::sleep_for(chrono::milliseconds(dist(rng)));
		lk.lock();
		/*
			readers reduced to 0
		*/
		readers_count[m[s]] = 0;
		reader_cond[m[s]].notify_all();
		writer_cond[m[s]].notify_all();
		lk.unlock();
	}
	dic[ {id, s}] = 0;
	cout << "Unlock [" << id << "," << s << "]\n";
}

/*
	Helper function to upgrade read lock to write lock of transaction whose id is "id" on "s"
*/
void upgrade_to_write(int id, string s) {
	mt19937 rng;
	rng.seed(random_device()());
	uniform_int_distribution<mt19937::result_type> dist(0, 20);
	unique_lock<mutex> lk(l[m[s]], defer_lock);
	this_thread::sleep_for(chrono::milliseconds(dist(rng)));
	lk.lock();
	readers_count[m[s]]--;
	if (readers_count[m[s]] > 0) {
		writer_cond[m[s]].wait(lk, [s]() { return readers_count[m[s]] == 0; });
	}
	readers_count[m[s]] = -1;
	lk.unlock();
	dic[ {id, s}] = 2;
	cout << "W-lock [" << id << "," << s << "]\n";
}

/*
	function to read the state variables
*/
void get_data() {
	string s;
	cin.ignore(256, '\n');
	getline(cin, s);
	stringstream t(s);
	string a, b;
	bool flag = false;
	while (t >> a) {
		if (!flag) b = a;
		else data[m[b]] = stoi(a);
		flag = !flag;
	}
}

/*
	function to execute transaction
*/
void exec_trans(transaction t) {
	/*
		his stores the state variable before this transaction
		and upd stores the state variables after this transaction
	*/
	map <string, int> his;
	map <string, int> upd;
	set <string> var = {"a", "b", "c", "d", "e"};
	set <string> lk;
	for (auto op : t.ops) {
		if (op.size() == 3) {
			if (op[0] == 'R') {
				string s;
				s += op[2];
				aquire_read_lock(t.id, s);
				his[s] = data[m[s]];
				upd[s] = data[m[s]];
				lk.insert(s);
			}
			else {
				string s;
				s += op[2];
				if (lk.find(s) != lk.end()) upgrade_to_write(t.id, s);
				else aquire_write_lock(t.id, s);
				data[m[s]] = upd[s];
				lk.insert(s);
			}
		}
		else {
			string s1, s2, s3;
			bool sign;
			int i = 0;
			while (op[i] != '=') {
				s1 += op[i];
				i++;
			}
			i++;
			while (op[i] != '+' and op[i] != '-') {
				s2 += op[i];
				i++;
			}
			if (op[i] == '+') sign = true;
			else sign = false;
			i++;
			while (i < op.size()) {
				s3 += op[i];
				i++;
			}
			if (sign) {
				if (var.find(s2) != var.end() and var.find(s3) != var.end()) {
					upd[s1] = upd[s2] + upd[s3];
				}
				else if (var.find(s2) != var.end()) {
					upd[s1] = upd[s2] + stoi(s3);
				}
				else if (var.find(s3) != var.end()) {
					upd[s1] = stoi(s2) + upd[s3];
				}
				else {
					upd[s1] = stoi(s2) + stoi(s3);
				}
			}
			else {
				if (var.find(s2) != var.end() and var.find(s3) != var.end()) {
					upd[s1] = upd[s2] - upd[s3];
				}
				else if (var.find(s2) != var.end()) {
					upd[s1] = upd[s2] - stoi(s3);
				}
				else if (var.find(s3) != var.end()) {
					upd[s1] = stoi(s2) - upd[s3];
				}
				else {
					upd[s1] = stoi(s2) - stoi(s3);
				}
			}
		}
	}
	/*
		if the transaction aborts updating the state variables to their original value
		and in the end releasing all the locks held by this transaction
	*/
	if (t.outcome == "A") {
		for (auto i : his) data[m[i.first]] = i.second;
	}
	for (auto i : lk) release_lock(t.id, i);
}

int main() {
#ifndef ONLINE_JUDGE
	freopen("input.txt", "r", stdin);
	freopen("output.txt", "w", stdout);
#endif
	// n is the number of transactions
	int n;
	cin >> n;

	// reading the state variables
	get_data();

	// vec is the vector of transactions
	vector <transaction> vec;
	int tid;
	for (int i = 0; i < n; i++) {
		cin >> tid;
		transaction t;
		t.id = tid;
		string s;
		cin.ignore(256, '\n');
		while (true) {
			getline(cin, s);
			if (s == "A" or s == "C") break;
			t.ops.push_back(s);
		}
		t.outcome = s;
		vec.push_back(t);
	}

	// seperate thread for each transaction
	vector <thread> threads;
	for (int i = 0; i < n; i++) threads.push_back(thread{exec_trans, vec[i]});
	for (int i = 0; i < n; i++) threads[i].join();

	// printing the final state variables
	for (auto i : m) cout << i.first << " " << data[i.second] << "\n";
	cout << "Successfully Completed\n";
	return 0;
}