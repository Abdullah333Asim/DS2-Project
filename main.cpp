#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <sstream>

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
                // Ignore duplicate words or "undelete" if it was previously marked as deleted
                if (distance == 0){
                    current->isDeleted = false;
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
            
            while (true){
                int distance = calculateLevenshteinDistance(current->word, word);
                // If we found the word, mark it as deleted
                if (distance == 0){
                    if (current->isDeleted){
                        return false; 
                    }
                    current->isDeleted = true; 
                    return true;
                }
                // If no child exists at this distance, the word is not in the tree
                if (current->children.find(distance) != current->children.end()){
                    current = current->children[distance];
                } else {
                    return false; 
                }
            }
        }

        // Recursive function to find suggestions within the tolerance range
        void searchHelper(BKNode* node, const string& query, int tolerance, vector<SearchResult>& results){
            if (node == nullptr) return;

            int dist = calculateLevenshteinDistance(node->word, query);

            if (dist <= tolerance && !node->isDeleted){
                results.push_back({node->word, dist, node->frequency});
            }

            // Triangle Inequality constraints
            int minBound = dist- tolerance;
            int maxBound = dist+tolerance;
            // Explore children within the bounds
            for (auto const& pair : node->children){
                int edgeWeight = pair.first;
                BKNode* childNode = pair.second;
                // Only explore this child if it could potentially have valid suggestions
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
        return;
    }
    
    while (getline(file, line)){
        // traversing the line to separate the word and its frequency
        string wordStr = "";
        int i = 0;
        while (i < line.length() && line[i] != ' ' && line[i] != '\t'){
            wordStr += line[i];
            i++;
        }

        toLowerCase(wordStr);
        // extracting only the last frequency
        string count = "";
        int j = line.length() - 1;
        while (j >= 0 && line[j] != ' ' && line[j] != '\t'){
            count = line[j] + count; 
            j--;
        }

        long long frequency = stringToNumber(count); 
        
        if (frequency > 0 && wordStr.length() > 0){
            tree.insert(wordStr, frequency);
        }
    }
    
    file.close();
}

void loadBigrams(const string& filename, unordered_map<string, unordered_map<string, long long>>& bigramMap){
    ifstream file(filename);
    string line;

    if (!file.is_open()) return;
    
    while (getline(file, line)){
        stringstream ss(line);
        string w1, w2, count;
        // It reads the first chunk into w1, the second into w2, and the third into count.
        if (ss >> w1 >> w2 >> count){
            toLowerCase(w1);
            toLowerCase(w2);
            long long frequency = stringToNumber(count);
            
            if (frequency > 0){
                bigramMap[w1][w2] = frequency;
            }
        }
    }
    
    file.close();
}

// void loadBigrams(const string& filename, unordered_map<string, unordered_map<string, long long>>& bigramMap){
//     ifstream file(filename);
//     string line;

//     if (!file.is_open()) return;
    
//     while (getline(file, line)){
//         string w1 = "", w2 = "", count = "";
//         int wordState = 0; // 0 = writing w1, 1 = writing w2, 2 = writing the number

//         for (int i = 0; i < line.length(); i++) {
//             char c = line[i];
            
//             if (c == ' ' || c == '\t') {
//                 if (wordState == 0 && w1.length() > 0) wordState = 1;
//                 else if (wordState == 1 && w2.length() > 0) wordState = 2;
//             } else {
//                 if (wordState == 0) w1 += c;
//                 else if (wordState == 1) w2 += c;
//                 else count += c;
//             }
//         }

//         toLowerCase(w1);
//         toLowerCase(w2);
//         long long frequency = stringToNumber(count);
        
//         if (frequency > 0 && w1.length() > 0 && w2.length() > 0) {
//             bigramMap[w1][w2] = frequency;
//         }
//     }
    
//     file.close();
// }

// primary sort by edit distance, secondary by context score
bool rankSuggestions(const SearchResult& a, const SearchResult& b){
    if (a.distance != b.distance) {
        return a.distance < b.distance; 
    }
    return a.contextScore > b.contextScore; 
}

void application1(string userLine, BKTree& tree, unordered_map<string, unordered_map<string, long long>>& bigramMap, int tolerance) {
    string originalSentence[100];
    string correctedSentence[100];
    int wordCount = 0;
    bool typoDetected = false;
    string currentWord = "";
    // traverse the user input character by character to separate words and handle them one by one
    for (int i = 0; i <= userLine.length() && wordCount < 100; i++){
        if (i == userLine.length() || userLine[i] == ' ' || userLine[i] == '\t'){
            if (currentWord.length() > 0){
                originalSentence[wordCount] = currentWord;
                // If the word is numeric, we skip spellchecking and directly add it to the corrected sentence
                if (isNumeric(currentWord)){
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
                // Calculate context score for each suggestion
                for (int s = 0; s < suggestions.size(); s++){
                    long long wordPopularity = suggestions[s].frequency;
                    long long pairPop = 0;
                    if (previousWord != "") {
                        pairPop = bigramMap[previousWord][suggestions[s].word];
                    }
                    suggestions[s].contextScore = wordPopularity + (pairPop * 50000000LL); 
                }

                sort(suggestions.begin(), suggestions.end(), rankSuggestions);
                // If the best suggestion is an exact match, keep the original word
                if (suggestions.empty() || suggestions[0].distance == 0){
                    correctedSentence[wordCount] = currentWord; 
                }else{
                    // otherwise, we have a typo and we replace it with the best suggestion
                    typoDetected = true;
                    correctedSentence[wordCount] = suggestions[0].word; 
                }
                wordCount++;
                currentWord = ""; 
            }
        }else{
            currentWord += userLine[i];
        }
    }

    if (typoDetected){
        cout << "Did you mean: \"";
        for (int i = 0; i < wordCount; i++){
            cout << correctedSentence[i] << (i == wordCount - 1 ? "" : " ");
        }
        cout << "\"?";
    }else{
        cout << "All words spelled correctly!";
    }
}

//Add a new word to personal dictionary 
void application2(string userWord, BKTree& tree){
    if (isNumeric(userWord)){
        cout << "ERROR_NUMERIC";
        return;
    }
    
    toLowerCase(userWord);
    vector<SearchResult> exactMatch = tree.getSuggestions(userWord, 0);
    
    if (!exactMatch.empty()){
        cout << "EXISTS";
    }else{
        tree.insert(userWord, 50000000); 
        // Append the new word to the personal dictionary file
        ofstream outFile("personal_dict.txt", ios::app);
        if (outFile.is_open()){
            outFile << userWord << " " << 50000000 << "\n";
            outFile.close();
            cout << "ADDED";
        }else{
            cout << "ERROR_FILE";
        }
    }
}

void application3(string query, BKTree& tree, unordered_map<string, unordered_map<string, long long>>& bigramMap, int tolerance){
    string currentWord = "";
    string previousWord = "";
    
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
    
    for (int s = 0; s < suggestions.size(); s++) {
        long long wordPopularity = suggestions[s].frequency;
        long long pairPop = 0;
        
        if (previousWord != "") {
            pairPop = bigramMap[previousWord][suggestions[s].word];
        }
        
        suggestions[s].contextScore = wordPopularity + (pairPop * 50000000LL); 
    }

    sort(suggestions.begin(), suggestions.end(), rankSuggestions);

    if (suggestions.empty() || suggestions[0].distance == 0) {
        cout << "CORRECT";
    } else {
        int displayCount = min(5, (int)suggestions.size());
        for (int i = 0; i < displayCount; i++) {
            cout << suggestions[i].word << (i == displayCount - 1 ? "" : ",");
        }
    }
}

// ==========================================
// API FUNCTION FOR PYTHON GUI (Application 4)
// ==========================================
void application4(string previousWord, unordered_map<string, unordered_map<string, long long>>& bigramMap) {
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
        application1(query, spellCheckerTree, bigramMap, tolerance);
    } else if (mode == "2") {
        application2(query, spellCheckerTree);
    } else if (mode == "3") {
        application3(query, spellCheckerTree, bigramMap, tolerance);
    } else if (mode == "4") {
        application4(query, bigramMap);
    }
    
    return 0; 
}