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
                if (distance == 0){
                    current->isDeleted = false; // If it was previously deleted, we can "undelete" it by resetting the flag
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
                
                // we found the exact word
                if (distance == 0) {
                    if (current->isDeleted) {
                        return false; // it was already deleted previously
                    }
                    current->isDeleted = true; // set the deletion flag
                    return true;
                }
                
                // If it's not the current node, check if a child exists at this distance
                if (current->children.find(distance) != current->children.end()) {
                    current = current->children[distance]; // Move down the tree
                } else {
                    return false; // word doesnt exist in the tree
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
}

// ==========================================
// FEATURE 4: The Context/Bigram Parser
// ==========================================
void loadBigrams(const string& filename, unordered_map<string, unordered_map<string, long long>>& bigramMap) {
    ifstream file(filename);
    string line;

    if (!file.is_open()) {
        cerr << "Error: Could not open " << filename << "\n";
        return;
    }

    int countLoaded = 0;
    
    while (getline(file, line)) {
        string w1 = "", w2 = "", countStr = "";
        int wordState = 0; // 0 = writing w1, 1 = writing w2, 2 = writing the number

        for (int i = 0; i < line.length(); i++) {
            char c = line[i];
            
            // The file separates the words with a space, and the number with a tab '\t'
            if (c == ' ' || c == '\t') {
                // If we hit a space/tab, move to the next state (but ignore consecutive spaces)
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
            countLoaded++;
        }
    }
    
    file.close();
}

// Custom sort: Primary sort by edit distance, secondary by popularity
bool rankSuggestions(const SearchResult& a, const SearchResult& b) {
    if (a.distance != b.distance) {
        return a.distance < b.distance; 
    }
    return a.contextScore > b.contextScore;; 
}

void runApplication1(BKTree& tree,unordered_map<string, unordered_map<string, long long>>& bigramMap, int tolerance) {
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
                    // applying Context Probability Scoring
                    // If we are not on the first word, grab the previous word!
                    string previousWord = "";
                    if (wordCount > 0) {
                        previousWord = correctedSentence[wordCount - 1];
                        toLowerCase(previousWord); 
                    }

                    // Calculate the smart score for every suggestion
                    for (int s = 0; s < suggestions.size(); s++) {
                        long long standalonePop = suggestions[s].frequency;
                        long long pairPop = 0;

                        // If we have a previous word, check the Bigram map!
                        if (previousWord != "") {
                            pairPop = bigramMap[previousWord][suggestions[s].word];
                        }

                        // THE PROBABILITY MATH:
                        // We give the standalone word its base score, but we heavily weight the bigram context.
                        // If pairPop is 0, it relies entirely on its standalone popularity.
                        long long contextBonus = pairPop * 50000000LL; 
                        suggestions[s].contextScore = standalonePop + contextBonus;
                    }

                    // Now sort using our custom rule!
                    sort(suggestions.begin(), suggestions.end(), rankSuggestions);
                    // ==========================================

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

// ==========================================
// FEATURE 2: Add Custom Word to Dictionary
// ==========================================
void runApplication2(BKTree& tree) {
    cout << "\n--- Application 2: Personal Dictionary Manager ---\n";
    cout << "Type a new word to add (or 'exit' to go back): \n";

    string userWord;
    while (true) {
        cout << "\nAdd Word> ";
        cin >> userWord;

        if (userWord == "exit") {
            cin.ignore(); // Clear buffer before returning to main menu
            break;
        }

        if (isNumeric(userWord)) {
            cout << "[Error] Cannot add numbers to the dictionary.\n";
            continue;
        }

        toLowerCase(userWord);

        // Check if it already exists using a tolerance of 0
        vector<SearchResult> exactMatch = tree.getSuggestions(userWord, 0);

        if (!exactMatch.empty()) {
            cout << "[Notice] The word '" << userWord << "' is already in the dictionary!\n";
        } else {
            cout << "The word '" << userWord << "' was not found. Add it to your personal dictionary? (y/n): ";
            string choice;
            cin >> choice;

            if (choice == "y" || choice == "Y") {
                // 1. Add to the BK-Tree in memory 
                // We give it a massive base frequency so user-added words are always prioritized as top suggestions!
                tree.insert(userWord, 50000000); 

                // 2. Save it to a personal text file so it persists after the program closes
                ofstream outFile("personal_dict.txt", ios::app);
                if (outFile.is_open()) {
                    // Format: word frequency (matching our parser design)
                    outFile << userWord << " " << 50000000 << "\n";
                    outFile.close();
                }

                cout << "[Success] '" << userWord << "' has been added to your system!\n";
            } else {
                cout << "[Cancelled] Word not added.\n";
            }
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

                // FEATURE 4: Context Multiplier
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

    // Print ONLY the result so Python can read it easily
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
    
    // Check if it exists with 0 tolerance (exact match)
    vector<SearchResult> exactMatch = tree.getSuggestions(userWord, 0);
    
    if (!exactMatch.empty()) {
        cout << "EXISTS";
    } else {
        // Insert into tree with massive priority
        tree.insert(userWord, 50000000); 
        
        // Save to file
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
// API FUNCTION FOR PYTHON GUI (Application 3 - Context-Aware Dropdown)
// ==========================================
void runApplication3_Headless(string query, BKTree& tree, unordered_map<string, unordered_map<string, long long>>& bigramMap, int tolerance) {
    string currentWord = "";
    string previousWord = "";
    
    // Split the query into individual words
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
    
    // The word they are currently typing is the last word in the array
    currentWord = words.back();
    toLowerCase(currentWord);

    // If there is a word before it, grab it for the Bigram Context!
    if (words.size() > 1) {
        previousWord = words[words.size() - 2];
        toLowerCase(previousWord);
    }

    vector<SearchResult> suggestions = tree.getSuggestions(currentWord, tolerance);
    
    // ==========================================
    // THE MAGIC: Apply Context Score to Live Suggestions!
    // ==========================================
    for (int s = 0; s < suggestions.size(); s++) {
        long long standalonePop = suggestions[s].frequency;
        long long pairPop = 0;
        
        if (previousWord != "") {
            pairPop = bigramMap[previousWord][suggestions[s].word];
        }
        
        // Boost the suggestion if it forms a valid Bigram!
        suggestions[s].contextScore = standalonePop + (pairPop * 50000000LL); 
    }

    // Sort using your custom probability rule
    sort(suggestions.begin(), suggestions.end(), rankSuggestions);

    if (suggestions.empty() || suggestions[0].distance == 0) {
        cout << "CORRECT";
    } else {
        // Output a comma-separated list of the top 5 smartest suggestions
        int displayCount = min(5, (int)suggestions.size());
        for (int i = 0; i < displayCount; i++) {
            cout << suggestions[i].word << (i == displayCount - 1 ? "" : ",");
        }
    }
}

// ==========================================
// API FUNCTION FOR PYTHON GUI (Application 4 - Context Map Math)
// ==========================================
void runApplication4_Headless(string query, BKTree& tree, unordered_map<string, unordered_map<string, long long>>& bigramMap, int tolerance) {
    // We expect exactly two words from Python: "previous_word typo_word"
    int spaceIndex = query.find(' ');
    if (spaceIndex == string::npos) {
        cout << "ERROR";
        return;
    }

    string prevWord = query.substr(0, spaceIndex);
    string typoWord = query.substr(spaceIndex + 1);

    toLowerCase(prevWord);
    toLowerCase(typoWord);

    vector<SearchResult> suggestions = tree.getSuggestions(typoWord, tolerance);
    if (suggestions.empty() || suggestions[0].distance == 0) {
        cout << "NO_TYPO";
        return;
    }

    // Apply the Application 4 Context Math!
    for (int s = 0; s < suggestions.size(); s++) {
        long long standalonePop = suggestions[s].frequency;
        long long pairPop = bigramMap[prevWord][suggestions[s].word];
        suggestions[s].contextScore = standalonePop + (pairPop * 50000000LL); 
    }

    // Sort using your custom rankSuggestions logic
    sort(suggestions.begin(), suggestions.end(), rankSuggestions);

    // Output top 3 results formatted for Python: word,base,bigram,final|word,base,bigram,final
    int displayCount = min(3, (int)suggestions.size());
    for (int i = 0; i < displayCount; i++) {
        long long pairPop = bigramMap[prevWord][suggestions[i].word];
        cout << suggestions[i].word << "," 
             << suggestions[i].frequency << "," 
             << pairPop << "," 
             << suggestions[i].contextScore;
        if (i < displayCount - 1) cout << "|";
    }
}

int main(int argc, char* argv[]){
    BKTree spellCheckerTree;
    
    loadDictionary("years-100k.txt", spellCheckerTree);

    // ===============================================
    // NEW: Silently attempt to load personal dictionary
    // ===============================================
    ifstream checkFile("personal_dict.txt");
    if (checkFile.is_open()) {
        checkFile.close();
        loadDictionary("personal_dict.txt", spellCheckerTree);
    }
    // ===============================================

    unordered_map<string, unordered_map<string, long long>> bigramMap;
    loadBigrams("count_2w.txt", bigramMap);

    int tolerance = 2;
    // ===============================================
    // NEW: Python API Mode Listener
    // ===============================================
    if (argc > 1) {
        string mode = argv[1]; 
        string query = "";
        
        for (int i = 2; i < argc; i++) {
            query += argv[i];
            if (i < argc - 1) query += " ";
        }

        if (mode == "1") {
            runApplication1_Headless(query, spellCheckerTree, bigramMap, tolerance);
        } else if (mode == "2") {
            runApplication2_Headless(query, spellCheckerTree); // NEW!
        } else if (mode == "3") {
            runApplication3_Headless(query, spellCheckerTree, bigramMap, tolerance);
        } else if (mode == "4") {
            runApplication4_Headless(query, spellCheckerTree, bigramMap, tolerance);
        }
        return 0; 
    }
    // =============================================== 

    while (true) {
        cout << "\nTemporary Main Menu Until we create a proper GUI\n";
        cout << "=========================================\n";
        cout << "1. Run Application 1 (Sentence Autocorrect Engine)\n";
        cout << "2. Run Application 2 (Personal Dictionary Manager)\n";
        cout << "3. Run Application 3 (Ranked Word Suggestions)\n";
        cout << "4. Delete a Word (Instructor Demo)\n";
        cout << "0. Exit\n";
        cout << "Choose an option: ";
        
        string choice;
        cin >> choice;

        if (choice == "1") {
            runApplication1(spellCheckerTree, bigramMap, tolerance);
        } else if (choice == "2") {
            runApplication2(spellCheckerTree); // NEW OPTION CALL
        } else if (choice == "3") {
            runApplication3(spellCheckerTree, tolerance);
        } else if (choice == "4") {
            cout << "\nEnter word to delete: ";
            string delWord;
            cin >> delWord;
            toLowerCase(delWord); 
            
            if (spellCheckerTree.deleteWord(delWord)) {
                cout << "[Success] '" << delWord << "' has been deleted from the dictionary.\n";
            } else {
                cout << "[Error] '" << delWord << "' was not found or is already deleted.\n";
            }
        } else if (choice == "0") {
            cout << "Exiting program...\n";
            break;
        } else {
            cout << "Invalid choice. Please try again.\n";
        }
    }

    return 0;
}