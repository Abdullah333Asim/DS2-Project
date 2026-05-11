
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cctype>

using namespace std;

// BK-Tree Node
struct BKNode{
    string word;
    long long frequency;
    bool isDeleted;
    unordered_map<int, BKNode*> children;

    BKNode(string w, long long freq): word(w), frequency(freq), isDeleted(false){}
};

// Stores the results for sorting later
struct SearchResult{
    string word;
    int distance;
    long long frequency;
    long long contextScore;
};

// Helper function to convert a string to lowercase
void toLowerCase(string& str){
    for (int i = 0; i < str.length(); i++){
        str[i] = tolower(str[i]);
    }
}

// Helper function to check if a word is just a number
bool isNumeric(const string& str) {
    if (str.empty()) return false;
    for (int i = 0; i < str.length(); i++){
        if (!isdigit(str[i])) return false; 
    }
    return true;
}

// Levenshtein Distance algo 
int calculateLevenshteinDistance(const string& s1, const string& s2){
    int m = s1.length();
    int n = s2.length();
    
    if (m > 99 || n > 99) return 999; 

    int arr[100][100]; 

    for (int i = 0; i <= m; i++){
        for (int j = 0; j <= n; j++){
            if (i == 0) {
                arr[i][j] = j; 
            } else if (j ==0){
                arr[i][j] = i; 
            } else if (s1[i-1] == s2[j- 1]){
                arr[i][j] = arr[i -1][j -1];
            } else {
                arr[i][j] = 1 + min({
                    arr[i][j -1],      // insert
                    arr[i-1][j],      // remove
                    arr[i-1][j-1]      // replace
                });
            }
        }
    }
    return arr[m][n];
}

class BKTree {
    private:
        BKNode* root;

    public:
        BKTree() : root(nullptr){}

        void insert(const string& word, long long frequency){
            if (root == nullptr) {
                root = new BKNode(word, frequency);
                return;
            }
            
            BKNode* current = root;
            
            while (true) {
                int distance = calculateLevenshteinDistance(current->word, word);
                // Ignore duplicate words
                if (distance == 0){
                    current->isDeleted = false; // "undelete" it by resetting the flag
                    return;
                } 
                
                // If no child exists at this distance, add the new node here
                if (current->children.find(distance) == current->children.end()){
                    current->children[distance] = new BKNode(word, frequency);
                    break;
                } else {
                    // Move down the tree
                    current = current->children[distance];
                }
            }
        }

        bool deleteWord(const string& word){
            if (root == nullptr) return false;
            BKNode* current = root;
            
            while (true) {
                int distance = calculateLevenshteinDistance(current->word, word);
                
                if (distance == 0) {
                    if (current->isDeleted) {
                        return false; 
                    }
                    current->isDeleted = true; 
                    return true;
                }
                
                if (current->children.find(distance) != current->children.end()) {
                    current = current->children[distance]; 
                } else {
                    return false; 
                }
            }
        }

        // Recursive function to find suggestions within the tolerance range
        void searchHelper(BKNode* node, const string& query, int tolerance, vector<SearchResult>& results) {
            if (node == nullptr) return;

            int dist = calculateLevenshteinDistance(node->word, query);

            if (dist <= tolerance && !node->isDeleted){
                results.push_back({node->word, dist, node->frequency});
            }

            // Triangle Inequality constraints
            int minBound = dist- tolerance;
            int maxBound = dist+tolerance;
            
            for (auto const& pair : node->children){
                int edgeWeight = pair.first;
                BKNode* childNode = pair.second;

                if (edgeWeight >= minBound && edgeWeight <= maxBound){
                    searchHelper(childNode, query, tolerance, results);
                }
            }
        }

        vector<SearchResult> getSuggestions(const string& query, int tolerance){
            vector<SearchResult> results;
            searchHelper(root, query, tolerance, results);
            return results;
        }
};

long long stringToNumber(const string& str){
    long long result = 0;
    for (int i = 0; i < str.length(); i++){
        if (isdigit(str[i])){
            result = (result * 10) + (str[i] - '0'); 
        }
    }
    return result;
}

void loadDictionary(const string& filename, BKTree& tree){
    ifstream file(filename);
    string line;

    if (!file.is_open()){
        return; // Silent fail for backend API
    }
    
    while (getline(file, line)) {
        string wordStr = "";
        int i = 0;
        while (i < line.length() && line[i] != ' ' && line[i] != '\t') {
            wordStr += line[i];
            i++;
        }

        toLowerCase(wordStr);

        string countStr = "";
        int j = line.length() - 1;
        while (j >= 0 && line[j] != ' ' && line[j] != '\t') {
            countStr = line[j] + countStr; 
            j--;
        }

        long long frequency = stringToNumber(countStr); 
        
        if (frequency > 0 && wordStr.length() > 0) {
            tree.insert(wordStr, frequency);
        }
    }
    
    file.close();
}

// ==========================================
// FEATURE 4: The Context/Bigram Parser
// ==========================================
void loadBigrams(const string& filename, unordered_map<string, unordered_map<string, long long>>& bigramMap) {
    ifstream file(filename);
    string line;

    if (!file.is_open()) return;
    
    while (getline(file, line)) {
        string w1 = "", w2 = "", countStr = "";
        int wordState = 0; // 0 = writing w1, 1 = writing w2, 2 = writing the number

        for (int i = 0; i < line.length(); i++) {
            char c = line[i];
            
            if (c == ' ' || c == '\t') {
                if (wordState == 0 && w1.length() > 0) wordState = 1;
                else if (wordState == 1 && w2.length() > 0) wordState = 2;
            } else {
                if (wordState == 0) w1 += c;
                else if (wordState == 1) w2 += c;
                else countStr += c;
            }
        }

        toLowerCase(w1);
        toLowerCase(w2);
        long long frequency = stringToNumber(countStr);
        
        if (frequency > 0 && w1.length() > 0 && w2.length() > 0) {
            bigramMap[w1][w2] = frequency;
        }
    }
    
    file.close();
}

// Custom sort: Primary sort by edit distance, secondary by context score
bool rankSuggestions(const SearchResult& a, const SearchResult& b) {
    if (a.distance != b.distance) {
        return a.distance < b.distance; 
    }
    return a.contextScore > b.contextScore; 
}

// ==========================================
// API FUNCTION FOR PYTHON GUI (Application 1)
// ==========================================
void runApplication1_Headless(string userLine, BKTree& tree, unordered_map<string, unordered_map<string, long long>>& bigramMap, int tolerance) {
    string originalSentence[100];
    string correctedSentence[100];
    int wordCount = 0;
    bool typoDetected = false;
    string currentWord = "";

    for (int i = 0; i <= userLine.length() && wordCount < 100; i++) {
        if (i == userLine.length() || userLine[i] == ' ' || userLine[i] == '\t') {
            if (currentWord.length() > 0) {
                originalSentence[wordCount] = currentWord;
                
                if (isNumeric(currentWord)) {
                    correctedSentence[wordCount] = currentWord;
                    wordCount++;
                    currentWord = ""; 
                    continue; 
                }

                string searchWord = currentWord;
                toLowerCase(searchWord);
                vector<SearchResult> suggestions = tree.getSuggestions(searchWord, tolerance);

                string previousWord = "";
                if (wordCount > 0) {
                    previousWord = correctedSentence[wordCount - 1];
                    toLowerCase(previousWord); 
                }

                for (int s = 0; s < suggestions.size(); s++) {
                    long long standalonePop = suggestions[s].frequency;
                    long long pairPop = 0;
                    if (previousWord != "") {
                        pairPop = bigramMap[previousWord][suggestions[s].word];
                    }
                    suggestions[s].contextScore = standalonePop + (pairPop * 50000000LL); 
                }

                sort(suggestions.begin(), suggestions.end(), rankSuggestions);

                if (suggestions.empty() || suggestions[0].distance == 0) {
                    correctedSentence[wordCount] = currentWord; 
                } else {
                    typoDetected = true;
                    correctedSentence[wordCount] = suggestions[0].word; 
                }
                wordCount++;
                currentWord = ""; 
            }
        } else {
            currentWord += userLine[i];
        }
    }

    if (typoDetected) {
        cout << "Did you mean: \"";
        for (int i = 0; i < wordCount; i++) {
            cout << correctedSentence[i] << (i == wordCount - 1 ? "" : " ");
        }
        cout << "\"?";
    } else {
        cout << "All words spelled correctly!";
    }
}

// ==========================================
// API FUNCTION FOR PYTHON GUI (Application 2)
// ==========================================
void runApplication2_Headless(string userWord, BKTree& tree) {
    if (isNumeric(userWord)) {
        cout << "ERROR_NUMERIC";
        return;
    }
    
    toLowerCase(userWord);
    vector<SearchResult> exactMatch = tree.getSuggestions(userWord, 0);
    
    if (!exactMatch.empty()) {
        cout << "EXISTS";
    } else {
        tree.insert(userWord, 50000000); 
        
        ofstream outFile("personal_dict.txt", ios::app);
        if (outFile.is_open()) {
            outFile << userWord << " " << 50000000 << "\n";
            outFile.close();
            cout << "ADDED";
        } else {
            cout << "ERROR_FILE";
        }
    }
}

// ==========================================
// API FUNCTION FOR PYTHON GUI (Application 3 - Stratified Dropdown)
// ==========================================
void runApplication3_Headless(string query, BKTree& tree, unordered_map<string, unordered_map<string, long long>>& bigramMap, int tolerance) {
    string currentWord = "";
    string previousWord = "";
    
    // Split the query
    vector<string> words;
    string temp = "";
    for (char c : query) {
        if (c == ' ' || c == '\t') {
            if (!temp.empty()) { words.push_back(temp); temp = ""; }
        } else {
            temp += c;
        }
    }
    if (!temp.empty()) words.push_back(temp);

    if (words.empty()) return;
    
    currentWord = words.back();
    toLowerCase(currentWord);

    if (words.size() > 1) {
        previousWord = words[words.size() - 2];
        toLowerCase(previousWord);
    }

    vector<SearchResult> suggestions = tree.getSuggestions(currentWord, tolerance);
    
    bool isCorrect = false;
    vector<SearchResult> bucketDist1;
    vector<SearchResult> bucketDist2;

    // Process all suggestions and bucket them
    for (int s = 0; s < suggestions.size(); s++) {
        // If distance is 0, they spelled it right, no need to suggest anything!
        if (suggestions[s].distance == 0) {
            isCorrect = true;
            break;
        }
        
        long long standalonePop = suggestions[s].frequency;
        long long pairPop = 0;
        
        if (previousWord != "") {
            pairPop = bigramMap[previousWord][suggestions[s].word];
        }
        
        suggestions[s].contextScore = standalonePop + (pairPop * 50000000LL); 

        // BUCKET THE WORDS
        if (suggestions[s].distance == 1) {
            bucketDist1.push_back(suggestions[s]);
        } else if (suggestions[s].distance == 2) {
            bucketDist2.push_back(suggestions[s]);
        }
    }

    if (isCorrect) {
        cout << "CORRECT";
        return;
    }

    // Sort both buckets independently using purely their Context Score!
    sort(bucketDist1.begin(), bucketDist1.end(), [](const SearchResult& a, const SearchResult& b){
        return a.contextScore > b.contextScore;
    });
    sort(bucketDist2.begin(), bucketDist2.end(), [](const SearchResult& a, const SearchResult& b){
        return a.contextScore > b.contextScore;
    });

    vector<SearchResult> finalSuggestions;
    
    // 1. Grab up to 3 words from Bucket 1
    int d1_count = min(3, (int)bucketDist1.size());
    for(int i = 0; i < d1_count; i++) {
        finalSuggestions.push_back(bucketDist1[i]);
    }

    // 2. Grab the remaining slots (up to 5 total rows) from Bucket 2
    int slots_left = 5 - finalSuggestions.size();
    int d2_count = min(slots_left, (int)bucketDist2.size());
    for(int i = 0; i < d2_count; i++) {
        finalSuggestions.push_back(bucketDist2[i]);
    }

    // Output the final mixed list!
    if (!finalSuggestions.empty()) {
        for (int i = 0; i < finalSuggestions.size(); i++) {
            cout << finalSuggestions[i].word << (i == finalSuggestions.size() - 1 ? "" : ",");
        }
    }
}

// ==========================================
// API FUNCTION FOR PYTHON GUI (Application 4)
// ==========================================
void runApplication4_Headless(string previousWord, unordered_map<string, unordered_map<string, long long>>& bigramMap) {
    toLowerCase(previousWord);

    if (bigramMap.find(previousWord) == bigramMap.end() || bigramMap[previousWord].empty()) {
        cout << "NO_PREDICTIONS";
        return;
    }

    vector<pair<string, long long>> nextWords;
    for (auto const& pair : bigramMap[previousWord]) {
        nextWords.push_back(pair);
    }

    sort(nextWords.begin(), nextWords.end(), [](const pair<string, long long>& a, const pair<string, long long>& b) {
        return a.second > b.second;
    });

    int displayCount = min(5, (int)nextWords.size());
    for (int i = 0; i < displayCount; i++) {
        cout << nextWords[i].first << (i == displayCount - 1 ? "" : ",");
    }
}

int main(int argc, char* argv[]){
    // If double-clicked manually by a user, provide a warning and close
    if (argc <= 1) {
        cout << "This is the backend API engine. Please run the Python GUI (gui.py) instead!" << endl;
        return 0;
    }

    BKTree spellCheckerTree;
    loadDictionary("years-100k.txt", spellCheckerTree);

    ifstream checkFile("personal_dict.txt");
    if (checkFile.is_open()) {
        checkFile.close();
        loadDictionary("personal_dict.txt", spellCheckerTree);
    }

    unordered_map<string, unordered_map<string, long long>> bigramMap;
    loadBigrams("count_2w.txt", bigramMap);

    int tolerance = 2;
    
    // API Router
    string mode = argv[1]; 
    string query = "";
    
    for (int i = 2; i < argc; i++) {
        query += argv[i];
        if (i < argc - 1) query += " ";
    }

    if (mode == "1") {
        runApplication1_Headless(query, spellCheckerTree, bigramMap, tolerance);
    } else if (mode == "2") {
        runApplication2_Headless(query, spellCheckerTree);
    } else if (mode == "3") {
        runApplication3_Headless(query, spellCheckerTree, bigramMap, tolerance);
    } else if (mode == "4") {
        runApplication4_Headless(query, bigramMap);
    }
    
    return 0; 
}