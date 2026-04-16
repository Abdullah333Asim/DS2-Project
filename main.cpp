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
    unordered_map<int, BKNode*> children;

    BKNode(string w, long long freq): word(w), frequency(freq){}
};

// Stores the results for sorting later
struct SearchResult{
    string word;
    int distance;
    long long frequency;
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
                    arr[i-1][j],      // Remove
                    arr[i-1][j-1]   // eplace
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
                if (distance == 0) return; 
                
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

        // Recursive function to find suggestions within the tolerance range
        void searchHelper(BKNode* node, const string& query, int tolerance, vector<SearchResult>& results) {
            if (node == nullptr) return;

            int dist = calculateLevenshteinDistance(node->word, query);

            if (dist <= tolerance){
                results.push_back({node->word, dist, node->frequency});
            }

            // Triangle Inequality constraints
            int minBound = dist- tolerance;
            int maxBound = dist+tolerance;
            // Only explore children that could potentially be within the tolerance range
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
        cout << "Could not open " << filename << "\n";
        return;
    }

    int countLoaded = 0;
    
    while (getline(file, line)) {
        // 1. Grab the word (Read from the start of the line until we hit a space or tab)
        string wordStr = "";
        int i = 0;
        while (i < line.length() && line[i] != ' ' && line[i] != '\t') {
            wordStr += line[i];
            i++;
        }

        // Convert the extracted word to lowercase
        toLowerCase(wordStr);

        // 2. Grab the final score (Read backwards from the end of the line until we hit a space or tab)
        string countStr = "";
        int j = line.length() - 1;
        while (j >= 0 && line[j] != ' ' && line[j] != '\t') {
            // Because we are reading backwards, we add the character to the FRONT of our string
            countStr = line[j] + countStr; 
            j--;
        }

        // 3. Convert the string to a number using our custom function!
        long long frequency = stringToNumber(countStr); 
        
        // 4. Insert into the tree
        if (frequency > 0 && wordStr.length() > 0) {
            tree.insert(wordStr, frequency);
            countLoaded++;
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

void runApplication1(BKTree& tree, int tolerance) {
    cout << "\nApplication 1: Smart Sentence Autocorrect\n";
    cout << "Type a full search query (or 'exit' to go back): \n";

    string userLine;
    while (true) {
        cout << "\nSearch> ";
        getline(cin >> ws, userLine); 

        if (userLine == "exit") break;

        string originalSentence[100];
        string correctedSentence[100];
        int wordCount = 0;
        bool typoDetected = false;

        string currentWord = "";

        // split the sentence into words character by character
        for (int i = 0; i <= userLine.length() && wordCount < 100; i++) {
            
            // If we hit a space, a tab, or the end of the sentence, we have a complete word
            if (i == userLine.length() || userLine[i] == ' ' || userLine[i] == '\t') {
                
                // Only process if the word isn't empty (handles extra spaces)
                if (currentWord.length() > 0) {
                    originalSentence[wordCount] = currentWord;

                    if (isNumeric(currentWord)) {
                        correctedSentence[wordCount] = currentWord;
                        wordCount++;
                        currentWord = ""; // Reset for the next word
                        continue; 
                    }

                    string searchWord = currentWord;
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
                    currentWord = ""; // Reset for the next word
                }
            } else {
                // Build the word one character at a time
                currentWord += userLine[i];
            }
        }

        // Display the final result and prompt the user
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

void runApplication3(BKTree& tree, int tolerance) {
    cout << "\nApplication 3: Ranked Suggestions\n";
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
            
            int displayCount = min(10, (int)suggestions.size());
            for (int i = 1; i < displayCount; i++) { 
                cout << "  " << i << ". " << suggestions[i].word << " (Edits: " << suggestions[i].distance << ", Popularity: " << suggestions[i].frequency << ")\n";
            }
        }
    }
}

int main() {
    BKTree spellCheckerTree;
    
    cout << "Loading dictionary, Please wait...\n";
    loadDictionary("years-100k.txt", spellCheckerTree);

    int tolerance = 2; 

    while (true) {
        cout << "\nTemporary Main Menu Until we create a proper GUI\n";
        cout << "=========================================\n";
        cout << "1. Run Application 1 (Sentence Autocorrect Engine)\n";
        cout << "2. Run Application 3 (Ranked Word Suggestions)\n";
        cout << "3. Exit\n";
        cout << "Choose an option: ";
        
        string choice;
        cin >> choice;

        if (choice == "1") {
            runApplication1(spellCheckerTree, tolerance);
        } else if (choice == "2") {
            runApplication3(spellCheckerTree, tolerance);
        } else if (choice == "3") {
            cout << "Exiting program...\n";
            break;
        } else {
            cout << "Invalid choice. Please try again.\n";
        }
    }

    return 0;
}