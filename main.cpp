#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cctype> // Required for std::tolower

// ==========================================
// 1. Core Data Structure: The BK-Tree Node
// ==========================================
struct BKNode {
    std::string word;
    long long frequency;
    
    // The key is the edit distance.
    // The value is the pointer to the child node.
    std::unordered_map<int, BKNode*> children;

    // Constructor
    BKNode(std::string w, long long freq) : word(w), frequency(freq) {}
};

// ==========================================
// 2. The Result Container (MUST BE ABOVE BKTREE)
// ==========================================
struct SearchResult {
    std::string word;
    int distance;
    long long frequency;
};

// ==========================================
// 3. The Math: Levenshtein Distance (Dynamic Programming)
// ==========================================
// ==========================================
// 3. The Math: Levenshtein Distance (OPTIMIZED)
// ==========================================
int calculateLevenshteinDistance(const std::string& s1, const std::string& s2) {
    int m = s1.length();
    int n = s2.length();
    
    // Safety check: Prevent overflow for ridiculously long accidental inputs
    if (m > 99 || n > 99) return 999; 

    // FAST STACK ALLOCATION: No more slow std::vector memory requests!
    int dp[100][100]; 

    for (int i = 0; i <= m; i++) {
        for (int j = 0; j <= n; j++) {
            if (i == 0) {
                dp[i][j] = j; 
            }
            else if (j == 0) {
                dp[i][j] = i; 
            }
            else if (s1[i - 1] == s2[j - 1]) {
                dp[i][j] = dp[i - 1][j - 1];
            }
            else {
                dp[i][j] = 1 + std::min({
                    dp[i][j - 1],      // Insert
                    dp[i - 1][j],      // Remove
                    dp[i - 1][j - 1]   // Replace
                });
            }
        }
    }
    return dp[m][n];
}

// ==========================================
// 4. The BK-Tree Class 
// ==========================================
class BKTree {
private:
    BKNode* root;

public:
    BKTree() : root(nullptr) {}

    // Insert logic
    void insert(const std::string& word, long long frequency) {
        // 1. If the tree is empty, the first word becomes the root
        if (root == nullptr) {
            root = new BKNode(word, frequency);
            return;
        }
        
        BKNode* current = root;
        
        // 2. Traverse the tree to find the right spot
        while (true) {
            // Calculate the edit distance between the current node and the new word
            int distance = calculateLevenshteinDistance(current->word, word);
            
            // If distance is 0, the word is exactly the same. We can ignore duplicates.
            if (distance == 0) {
                return; 
            }
            
            // Check if the current node already has a child at this exact distance
            if (current->children.find(distance) == current->children.end()) {
                // NO CHILD EXISTS at this distance! 
                // We found the correct spot. Create the new node here and stop.
                current->children[distance] = new BKNode(word, frequency);
                break;
            } else {
                // A CHILD ALREADY EXISTS at this distance!
                // Move down to that child and repeat the loop.
                current = current->children[distance];
            }
        }
    }

    // This is the recursive DFS function that heavily prunes the tree
    void searchHelper(BKNode* node, const std::string& query, int tolerance, std::vector<SearchResult>& results) {
        if (node == nullptr) return;

        // How far is the current node from our misspelled word?
        int dist = calculateLevenshteinDistance(node->word, query);

        // If it falls within our acceptable typo range, add it to our results!
        if (dist <= tolerance) {
            results.push_back({node->word, dist, node->frequency});
        }

        // Triangle Inequality: Calculate our boundaries
        int minBound = dist - tolerance;
        int maxBound = dist + tolerance;

        // ONLY visit children whose edge weights fall within this mathematical boundary
        for (auto const& pair : node->children) {
            int edgeWeight = pair.first;
            BKNode* childNode = pair.second;

            if (edgeWeight >= minBound && edgeWeight <= maxBound) {
                searchHelper(childNode, query, tolerance, results);
            }
        }
    }

    // This is the public function we will call from our main loop
    std::vector<SearchResult> getSuggestions(const std::string& query, int tolerance) {
        std::vector<SearchResult> results;
        searchHelper(root, query, tolerance, results);
        return results;
    }
};


// ==========================================
// 5. The CSV Parser
// ==========================================
// ==========================================
// 5. The TXT Parser (Updated for years-100k.txt)
// ==========================================
void loadDictionary(const std::string& filename, BKTree& tree) {
    std::ifstream file(filename);
    std::string line;

    if (!file.is_open()) {
        std::cerr << "Error: Could not open the file " << filename << "\n";
        return;
    }

    int countLoaded = 0;
    
    // Read the file line by line
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string wordStr;
        std::string tempStr;
        std::string lastCountStr;

        // 1. Extract the first column (the word)
        if (ss >> wordStr) {
            
            // 2. Convert the ALL CAPS word to lowercase
            std::transform(wordStr.begin(), wordStr.end(), wordStr.begin(),
                           [](unsigned char c){ return std::tolower(c); });

            // 3. Keep reading the remaining columns until the end of the line
            // The last thing stored in tempStr will be your most recent popularity score
            while (ss >> tempStr) {
                lastCountStr = tempStr;
            }

            // 4. Insert into the BK-Tree
            try {
                // We no longer need the 50,000 threshold because this file only has correct words!
                long long frequency = std::stoll(lastCountStr); 
                tree.insert(wordStr, frequency);
                countLoaded++;
            } catch (const std::exception& e) {
                // Ignore lines with corrupted numbers
                continue; 
            }
        }
    }

    file.close();
    std::cout << "Successfully loaded " << countLoaded << " words into the dictionary.\n";
}

// ==========================================
// 6. Sorting / Ranking Rule
// ==========================================
// 1. Smallest edit distance first (closest match)
// 2. If distance is tied, Highest frequency first (most popular word)
bool rankSuggestions(const SearchResult& a, const SearchResult& b) {
    if (a.distance != b.distance) {
        return a.distance < b.distance; 
    }
    return a.frequency > b.frequency; 
}

// Helper function to check if a word is just a number
bool isNumeric(const std::string& str) {
    if (str.empty()) return false;
    for (char c : str) {
        if (!std::isdigit(c)) {
            return false; // Found a letter or symbol, so it's not a pure number
        }
    }
    return true;
}

// ==========================================
// 7. Main Execution
// ==========================================
int main() {
    BKTree spellCheckerTree;
    
    std::cout << "Loading dictionary... Please wait...\n";
    // UPDATE THIS WITH YOUR ABSOLUTE PATH
    loadDictionary("C:\\Users\\HP\\Desktop\\DS2-Project\\years-100k.txt", spellCheckerTree);

    int tolerance = 2; // Allow up to 2 typos

    std::cout << "\n--- Smart Autocorrect Engine Online ---\n";
    std::cout << "Type a full search query (or 'exit' to quit): \n";

    std::string userLine;
    
    while (true) {
        std::cout << "\nSearch> ";
        
        // Use getline instead of cin so we can capture spaces and full sentences!
        std::getline(std::cin >> std::ws, userLine); 

        if (userLine == "exit") break;

        std::stringstream ss(userLine);
        std::string currentWord;
        
        std::vector<std::string> originalSentence;
        std::vector<std::string> correctedSentence;
        bool typoDetected = false;

        // 1. Process the sentence word by word
        while (ss >> currentWord) {
            originalSentence.push_back(currentWord);

            // --- THE NEW NUMBER BYPASS ---
            // If the word is a number, accept it as correct and skip the search!
            if (isNumeric(currentWord)) {
                correctedSentence.push_back(currentWord);
                continue; 
            }
            // -----------------------------

            // Convert word to lowercase for the BK-Tree search
            std::string searchWord = currentWord;
            std::transform(searchWord.begin(), searchWord.end(), searchWord.begin(),
                           [](unsigned char c){ return std::tolower(c); });

            // Fetch and rank suggestions
            std::vector<SearchResult> suggestions = spellCheckerTree.getSuggestions(searchWord, tolerance);
            std::sort(suggestions.begin(), suggestions.end(), rankSuggestions);

            // 2. Decide what to do with the word
            if (suggestions.empty()) {
                // No clue what this is, leave it alone
                correctedSentence.push_back(currentWord); 
            } else if (suggestions[0].distance == 0) {
                // Exact match! It is spelled correctly.
                correctedSentence.push_back(currentWord); 
            } else {
                // Typo found! Replace it with the #1 ranked suggestion
                typoDetected = true;
                correctedSentence.push_back(suggestions[0].word); 
            }
        }

        // 3. Display the final result to the user
        if (typoDetected) {
            std::cout << "-> Did you mean: \"";
            for (size_t i = 0; i < correctedSentence.size(); i++) {
                std::cout << correctedSentence[i] << (i == correctedSentence.size() - 1 ? "" : " ");
            }
            std::cout << "\"?\n";
            
            // You can optionally print out the alternative choices for the specific typo here later!
        } else {
            std::cout << "-> Searching for: \"" << userLine << "\" (All words spelled correctly!)\n";
        }
    }

    return 0;
}