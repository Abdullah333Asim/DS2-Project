#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>

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
void loadDictionary(const std::string& filename, BKTree& tree) {
    std::ifstream file(filename);
    std::string line;

    if (!file.is_open()) {
        std::cerr << "Error: Could not open the file " << filename << "\n";
        return;
    }

    // Skip the header line ("word,count")
    std::getline(file, line);

    int countLoaded = 0;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string wordStr;
        std::string countStr;

        // Split the line by comma
        if (std::getline(ss, wordStr, ',') && std::getline(ss, countStr, ',')) {
            // Safety check: The dataset has a couple of empty/null words
            if (wordStr.empty() || countStr.empty()) continue;

            try {
                long long frequency = std::stoll(countStr); // Convert string to long long
                tree.insert(wordStr, frequency);
                countLoaded++;
            } catch (const std::exception& e) {
                // Ignore lines that have corrupted numbers
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

// ==========================================
// 7. Main Execution
// ==========================================
int main() {
    BKTree spellCheckerTree;
    
    std::cout << "Loading dictionary... Please wait...\n";
    
    // NOTE: Based on your last terminal output, double check if your file is in "Desktop\DS2-Project" 
    // or "Documents\GitHub\DS2 Project". Update this path to match exactly where the CSV is!
    loadDictionary("C:\\Users\\HP\\Desktop\\DS2-Project\\unigram_freq.csv", spellCheckerTree);

    std::string userInput;
    int tolerance = 2; // Allow up to 2 typos

    std::cout << "\n--- Smart Autocorrect Engine Online ---\n";
    std::cout << "Type a word to search (or 'exit' to quit): \n";

    while (true) {
        std::cout << "\nSearch> ";
        std::cin >> userInput;

        if (userInput == "exit") break;

        // 1. Fetch suggestions
        std::vector<SearchResult> suggestions = spellCheckerTree.getSuggestions(userInput, tolerance);

        // 2. Rank suggestions using our custom rule
        std::sort(suggestions.begin(), suggestions.end(), rankSuggestions);

        // 3. Display the results
        if (suggestions.empty()) {
            std::cout << "No matches found within " << tolerance << " typos.\n";
        } else if (suggestions[0].distance == 0) {
            std::cout << "[Exact Match Found]: " << userInput << " is spelled correctly!\n";
        } else {
            std::cout << "Did you mean: " << suggestions[0].word << "?\n";
            std::cout << "\nOther potential matches:\n";
            
            // Print up to 5 alternative suggestions
            int displayCount = std::min(5, (int)suggestions.size());
            for (int i = 0; i < displayCount; i++) {
                std::cout << i + 1 << ". " << suggestions[i].word 
                          << " (Edits: " << suggestions[i].distance 
                          << ", Popularity: " << suggestions[i].frequency << ")\n";
            }
        }
    }

    return 0;
}