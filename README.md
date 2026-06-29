# BARM: Predictive Search & Autocorrect Engine

BARM is a high-performance predictive search engine and autocorrect system that integrates advanced data structures with a modern Python graphical interface. The project bridges the computational efficiency of C++ (for core algorithmic heavy lifting) with the UI flexibility of Python (for user interaction).

## Key Features

* **Sentence Autocorrect:** Utilizes Levenshtein distance calculations on a BK-Tree to suggest corrections for entire strings.
* **Context-Aware Ranking:** Implements Bigram frequency analysis to boost the probability of word sequences that logically fit together.
* **Stratified Suggestion Engine:** Stratifies results by edit distance (Distance-1 vs. Distance-2) to ensure the highest-quality matches are prioritized.
* **Personal Dictionary Persistence:** Users can extend the vocabulary via a dynamic manager, with changes saved to `personal_dict.txt`.
* **Next-Word Prediction:** Uses a Markov-chain-style context map to predict the most likely subsequent word after a space is entered.

## Architecture: Headless API Approach

The project is decoupled into two distinct layers:

1. **Backend (C++):** A "headless" mathematical API that performs heavy computations (BK-Tree traversals, Levenshtein distance, Hash Map lookups). It receives commands from the frontend via CLI arguments and outputs results to `stdout`.
2. **Frontend (Python/CustomTkinter):** A modern UI that captures user keystrokes, debounces input, and manages the communication with the C++ backend as a subprocess.

## Performance Metrics (Time Complexity)

| Operation | Average Complexity | Worst Case |
| :--- | :--- | :--- |
| **Search/Autocorrect** | O(log N * L^2) | O(N * L^2) |
| **Dictionary Insert** | O(log N * L^2) | O(N * L^2) |
| **Bigram Lookup** | O(L) | O(L * N) |

*Where N is dictionary size and L is word length.*

## How to Run

### 1. Backend Compilation
Ensure you have `g++` installed. Compile the core C++ engine:

```bash
g++ main.cpp -o main.exe
```
### 2. Frontend Launch
Ensure customtkinter is installed:

```bash
pip install customtkinter
```

Launch the application using the full path to your Python installation:
```bash
python gui.py
```

## Data Sources

* **`years-100k.txt`:** Base dictionary for spelling corrections.
* **`count_2w.txt`:** Bigram dataset used for context-aware scoring.
* **`personal_dict.txt`:** User-generated dictionary persistence.

*Developed as a University Data Structures & Algorithms project.*
