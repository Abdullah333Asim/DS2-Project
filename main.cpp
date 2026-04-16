#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cctype>

using namespace std;

// BK-Tree Node
struct BKNode {
    string word;
    long long frequency;
    unordered_map<int, BKNode*> children;

    BKNode(string w, long long freq) : word(w), frequency(freq) {}
};

// Stores the results for sorting later
struct SearchResult {
    string word;
    int distance;
    long long frequency;
};

// Helper function to convert a string to lowercase
void toLowerCase(string& str) {
    for (int i = 0; i < str.length(); i++) {
        str[i] = tolower(str[i]);
    }
}

// Helper function to check if a word is just a number
bool isNumeric(const string& str) {
    if (str.empty()) return false;
    for (int i = 0; i < str.length(); i++) {
        if (!isdigit(str[i])) return false; 
    }
    return true;
}

// Optimized Levenshtein Distance using stack allocation
int calculateLevenshteinDistance(const string& s1, const string& s2) {
    int m = s1.length();
    int n = s2.length();
    
    // Safety check to prevent array overflow
    if (m > 99 || n > 99) return 999; 

    int dp[100][100]; 

    for (int i = 0; i <= m; i++) {
        for (int j = 0; j <= n; j++) {
            if (i == 0) {
                dp[i][j] = j; 
            } else if (j == 0) {
                dp[i][j] = i; 
            } else if (s1[i - 1] == s2[j - 1]) {
                dp[i][j] = dp[i - 1][j - 1];
            } else {
                dp[i][j] = 1 + min({
                    dp[i][j - 1],      // Insert
                    dp[i - 1][j],      // Remove
                    dp[i - 1][j - 1]   // Replace
                });
            }
        }
    }
    return dp[m][n];
}

class BKTree {
private:
    BKNode* root;

public:
    BKTree() : root(nullptr) {}

    void insert(const string& word, long long frequency) {
        if (root == nullptr) {
            root = new BKNode(word, frequency);
            return;
        }
        
        BKNode* current = root;
        
        while (true) {
            int distance = calculateLevenshteinDistance(current->word, word);
            
            // Ignore duplicate words
            if (distance == 0) return; 
            
            // If no child exists at this distance, add the new node here
            if (current->children.find(distance) == current->children.end()) {
                current->children[distance] = new BKNode(word, frequency);
                break;
            } else {
                // Move down the tree
                current = current->children[distance];
            }
        }
    }

    // Recursive DFS to find suggestions within the tolerance range
    void searchHelper(BKNode* node, const string& query, int tolerance, vector<SearchResult>& results) {
        if (node == nullptr) return;

        int dist = calculateLevenshteinDistance(node->word, query);

        if (dist <= tolerance) {
            results.push_back({node->word, dist, node->frequency});
        }

        // Triangle Inequality constraints
        int minBound = dist - tolerance;
        int maxBound = dist + tolerance;

        // Prune the tree: only visit children within our mathematical bounds
        for (auto const& pair : node->children) {
            int edgeWeight = pair.first;
            BKNode* childNode = pair.second;

            if (edgeWeight >= minBound && edgeWeight <= maxBound) {
                searchHelper(childNode, query, tolerance, results);
            }
        }
    }

    vector<SearchResult> getSuggestions(const string& query, int tolerance) {
        vector<SearchResult> results;
        searchHelper(root, query, tolerance, results);
        return results;
    }
};

void loadDictionary(const string& filename, BKTree& tree) {
    ifstream file(filename);
    string line;

    if (!file.is_open()) {
        cerr << "Error: Could not open " << filename << "\n";
        return;
    }

    int countLoaded = 0;
    
    while (getline(file, line)) {
        stringstream ss(line);
        string wordStr, tempStr, lastCountStr;

        if (ss >> wordStr) {
            // Replaced the complex transform() function with our simple helper
            toLowerCase(wordStr);

            // Grab the last column for the frequency score
            while (ss >> tempStr) {
                lastCountStr = tempStr;
            }

            try {
                long long frequency = stoll(lastCountStr); 
                tree.insert(wordStr, frequency);
                countLoaded++;
            } catch (...) {
                continue; 
            }
        }
    }
    file.close();
    cout << "Loaded " << countLoaded << " words into the dictionary.\n";
}

// Custom sort: Primary sort by edit distance, secondary by popularity
bool rankSuggestions(const SearchResult& a, const SearchResult& b) {
    if (a.distance != b.distance) {
        return a.distance < b.distance; 
    }
    return a.frequency > b.frequency; 
}

// ==========================================
// FEATURE 1: Sentence Autocorrect with UI Prompt
// ==========================================
void runApplication1(BKTree& tree, int tolerance) {
    cout << "\n--- Application 1: Smart Sentence Autocorrect ---\n";
    cout << "Type a full search query (or 'exit' to go back): \n";

    string userLine;
    while (true) {
        cout << "\nSearch> ";
        getline(cin >> ws, userLine); 

        if (userLine == "exit") break;

        stringstream ss(userLine);
        string currentWord;
        
        string originalSentence[100];
        string correctedSentence[100];
        int wordCount = 0;
        bool typoDetected = false;

        while (ss >> currentWord && wordCount < 100) {
            originalSentence[wordCount] = currentWord;

            if (isNumeric(currentWord)) {
                correctedSentence[wordCount] = currentWord;
                wordCount++;
                continue; 
            }

            string searchWord = currentWord;
            // Replaced transform() with simple helper
            toLowerCase(searchWord);

            vector<SearchResult> suggestions = tree.getSuggestions(searchWord, tolerance);
            sort(suggestions.begin(), suggestions.end(), rankSuggestions);

            if (suggestions.empty() || suggestions[0].distance == 0) {
                correctedSentence[wordCount] = currentWord; 
            } else {
                typoDetected = true;
                correctedSentence[wordCount] = suggestions[0].word; 
            }
            wordCount++;
        }

        if (typoDetected) {
            cout << "\n-> Did you mean: \"";
            for (int i = 0; i < wordCount; i++) {
                cout << correctedSentence[i] << (i == wordCount - 1 ? "" : " ");
            }
            cout << "\"?\n";
            
            cout << "\nPress 'y' to search with the corrected query, or 'n' to use your exact input: ";
            string choice;
            getline(cin, choice);

            if (choice == "y" || choice == "Y") {
                cout << "[Executing Search]: \"";
                for (int i = 0; i < wordCount; i++) {
                    cout << correctedSentence[i] << (i == wordCount - 1 ? "" : " ");
                }
                cout << "\"\n";
            } else {
                cout << "[Executing Search]: \"" << userLine << "\"\n";
            }
        } else {
            cout << "\n[Executing Search]: \"" << userLine << "\" (All words spelled correctly!)\n";
        }
    }
}

// ==========================================
// FEATURE 3: Single Word Ranking Display
// ==========================================
void runApplication3(BKTree& tree, int tolerance) {
    cout << "\n--- Application 3: Ranked Suggestions ---\n";
    cout << "Type a misspelled word to see rankings (or 'exit' to go back): \n";

    string userWord;
    while (true) {
        cout << "\nWord> ";
        cin >> userWord;

        if (userWord == "exit") {
            cin.ignore(); 
            break;
        }

        if (isNumeric(userWord)) {
            cout << "Skipping number.\n";
            continue;
        }

        string searchWord = userWord;
        // Replaced transform() with simple helper
        toLowerCase(searchWord);

        vector<SearchResult> suggestions = tree.getSuggestions(searchWord, tolerance);
        sort(suggestions.begin(), suggestions.end(), rankSuggestions);

        if (suggestions.empty()) {
            cout << "No matches found.\n";
        } else if (suggestions[0].distance == 0) {
            cout << "[Exact Match]: '" << userWord << "' is spelled correctly!\n";
        } else {
            cout << "\n[Typo Detected]: '" << userWord << "' -> Top Suggestion: '" << suggestions[0].word << "'\n";
            cout << "Other ranked suggestions:\n";
            
            int displayCount = min(5, (int)suggestions.size());
            for (int i = 1; i < displayCount; i++) { 
                cout << "  " << i << ". " << suggestions[i].word 
                     << " (Edits: " << suggestions[i].distance 
                     << ", Popularity: " << suggestions[i].frequency << ")\n";
            }
        }
    }
}

int main() {
    BKTree spellCheckerTree;
    
    cout << "Loading dictionary... Please wait...\n";
    loadDictionary("years-100k.txt", spellCheckerTree);

    int tolerance = 2; 

    while (true) {
        cout << "\n=====================================\n";
        cout << "        DS2 Project Main Menu        \n";
        cout << "=====================================\n";
        cout << "1. Run Application 1 (Sentence Autocorrect Engine)\n";
        cout << "3. Run Application 3 (Ranked Word Suggestions)\n";
        cout << "0. Exit\n";
        cout << "Choose an option: ";
        
        string choice;
        cin >> choice;

        if (choice == "1") {
            runApplication1(spellCheckerTree, tolerance);
        } else if (choice == "3") {
            runApplication3(spellCheckerTree, tolerance);
        } else if (choice == "0") {
            cout << "Exiting program...\n";
            break;
        } else {
            cout << "Invalid choice. Please try again.\n";
        }
    }

    return 0;
}