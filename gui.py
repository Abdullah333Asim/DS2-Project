import customtkinter as ctk
import subprocess

# Set up the modern dark theme (Google Dark Mode style)
ctk.set_appearance_mode("dark")
ctk.set_default_color_theme("blue")

app = ctk.CTk()
app.geometry("800x600")
app.title("Google Clone - NLP Search")

# --- Global Variables ---
typing_timer = None
executable_path = "./main.exe"
suggested_sentence = ""
dict_window = None # Tracks the popup window

# ==========================================
# FEATURE 2: Personal Dictionary Manager (Popup)
# ==========================================
def open_dictionary_manager():
    global dict_window
    
    # Prevent opening multiple windows if they click it twice
    if dict_window is not None and dict_window.winfo_exists():
        dict_window.focus()
        return

    # Create the floating popup window
    dict_window = ctk.CTkToplevel(app)
    dict_window.title("Dictionary Manager")
    dict_window.geometry("400x280")
    dict_window.attributes("-topmost", True) # Keep it on top of main window
    dict_window.configure(fg_color="#202124")

    # UI Elements inside the popup
    title = ctk.CTkLabel(dict_window, text="Teach a New Word", font=("Roboto", 20, "bold"), text_color="#E8EAED")
    title.pack(pady=(20, 10))

    subtitle = ctk.CTkLabel(dict_window, text="Add slang, names, or technical terms.", font=("Roboto", 13), text_color="#9AA0A6")
    subtitle.pack(pady=(0, 20))

    new_word_entry = ctk.CTkEntry(dict_window, placeholder_text="Type word here...", width=250, height=40, font=("Roboto", 15), fg_color="#303134", border_color="#5F6368")
    new_word_entry.pack(pady=5)

    status_label = ctk.CTkLabel(dict_window, text="", font=("Roboto", 13))
    status_label.pack(pady=5)

    def submit_new_word():
        word = new_word_entry.get().strip()
        if not word or " " in word:
            status_label.configure(text="Please enter a single valid word.", text_color="#FF6B6B")
            return
            
        status_label.configure(text="Processing...", text_color="#E8EAED")
        dict_window.update()

        try:
            # Call Application 2!
            process = subprocess.run([executable_path, "2", word], capture_output=True, text=True)
            output = process.stdout.strip()
            
            if "ADDED" in output:
                status_label.configure(text=f"Success! '{word}' is now permanently saved.", text_color="#6BFF8E")
                new_word_entry.delete(0, "end")
            elif "EXISTS" in output:
                status_label.configure(text=f"'{word}' is already in the dictionary.", text_color="#FFB366")
            elif "ERROR_NUMERIC" in output:
                status_label.configure(text="Numbers cannot be added.", text_color="#FF6B6B")
            else:
                status_label.configure(text="Error saving word.", text_color="#FF6B6B")
        except Exception as e:
            status_label.configure(text="Backend connection failed.", text_color="#FF6B6B")

    add_btn = ctk.CTkButton(dict_window, text="Add to Engine", width=150, height=35, fg_color="#8AB4F8", text_color="#202124", hover_color="#AECBFA", font=("Roboto", 14, "bold"), command=submit_new_word)
    add_btn.pack(pady=10)

# ==========================================
# FEATURE 3: Live Typing Suggestions
# ==========================================
def on_key_release(event):
    global typing_timer
    if event.keysym in ['Return', 'Shift_L', 'Shift_R', 'Up', 'Down', 'Left', 'Right', 'BackSpace']:
        if event.keysym == 'BackSpace' and len(search_entry.get()) == 0:
            hide_dropdown()
        return

    if typing_timer is not None:
        app.after_cancel(typing_timer)
    
    typing_timer = app.after(600, fetch_live_suggestions)

def fetch_live_suggestions():
    query = search_entry.get().strip()
    if not query:
        hide_dropdown()
        return

    # Split the sentence
    words = query.split()
    current_word = words[-1]
    previous_words = " ".join(words[:-1])
    
    # Save the exact original words so we can compare them later for BOLDING
    original_words_list = words 
    corrected_previous_words = previous_words

    # 1. Autocorrect the PREVIOUS words in the background (App 1)
    if previous_words:
        try:
            p1 = subprocess.run([executable_path, "1", previous_words], capture_output=True, text=True)
            out1 = p1.stdout.strip()
            if "Did you mean:" in out1:
                start_idx = out1.find('"') + 1
                end_idx = out1.rfind('"')
                corrected_previous_words = out1[start_idx:end_idx]
        except Exception as e:
            print("Backend Error (App 1):", e)

    # 2. Get 5 suggestions for the CURRENT typing word (App 3)
    try:
        p3 = subprocess.run([executable_path, "3", current_word], capture_output=True, text=True)
        out3 = p3.stdout.strip()
        
        if out3 == "CORRECT" or not out3:
            # Even if the last word is correct, the previous words might have been autocorrected!
            if previous_words != corrected_previous_words:
                show_dropdown([current_word], corrected_previous_words, original_words_list)
            else:
                hide_dropdown()
        else:
            suggestions = out3.split(',')
            show_dropdown(suggestions, corrected_previous_words, original_words_list)
    except Exception as e:
        print("Backend Error (App 3):", e)

# --- Dropdown UI Logic (Rich Text Builder) ---
def show_dropdown(suggestions, corrected_previous, original_words_list):
    # Clear old suggestions
    for widget in dropdown_frame.winfo_children():
        widget.destroy()

    corrected_prev_list = corrected_previous.split() if corrected_previous else []

    for word in suggestions:
        full_suggestion_text = f"{corrected_previous} {word}".strip()
        suggested_words_list = corrected_prev_list + [word]

        # 1. Create a custom invisible frame to act as our "Button"
        row_frame = ctk.CTkFrame(dropdown_frame, fg_color="transparent", corner_radius=0)
        row_frame.pack(fill="x", padx=0, pady=2)

        # Keep track of everything inside this row so we can make them all clickable
        widgets_to_bind = [row_frame]

        # 2. Add the magnifying glass
        icon_label = ctk.CTkLabel(row_frame, text="🔍   ", font=("Roboto", 15), text_color="#E8EAED")
        icon_label.pack(side="left", padx=(10, 0), pady=8)
        widgets_to_bind.append(icon_label)

        # 3. Build the Rich Text (Compare Original vs Suggested)
        for i, sugg_w in enumerate(suggested_words_list):
            is_bold = False
            
            # If the suggested word is different from what the user typed, make it BOLD!
            if i < len(original_words_list):
                if original_words_list[i].lower() != sugg_w.lower():
                    is_bold = True
            else:
                is_bold = True # Catch-all for extra words

            font_style = ("Roboto", 15, "bold") if is_bold else ("Roboto", 15, "normal")
            text_color = "#FFFFFF" if is_bold else "#E8EAED"
            
            # Pack the words side-by-side horizontally
            word_label = ctk.CTkLabel(row_frame, text=sugg_w + " ", font=font_style, text_color=text_color)
            word_label.pack(side="left", pady=8)
            widgets_to_bind.append(word_label)

        # 4. Bind Hover and Click events to simulate a real button
        def on_enter(e, f=row_frame): f.configure(fg_color="#3C4043")
        def on_leave(e, f=row_frame): f.configure(fg_color="transparent")
        def on_click(e, t=full_suggestion_text): apply_suggestion(t)

        for w in widgets_to_bind:
            w.bind("<Enter>", on_enter)
            w.bind("<Leave>", on_leave)
            w.bind("<Button-1>", on_click)
            w.configure(cursor="hand2") # Turn mouse into a pointer
    
    dropdown_frame.pack(pady=0, fill="x", padx=150)

def hide_dropdown():
    dropdown_frame.pack_forget()

def apply_suggestion(full_text):
    search_entry.delete(0, "end")
    search_entry.insert(0, full_text + " ") 
    hide_dropdown()
    search_entry.focus()

# ==========================================
# FEATURE 1: Sentence Search & "Did You Mean"
# ==========================================
def execute_search(event=None):
    global suggested_sentence
    hide_dropdown()
    
    query = search_entry.get().strip()
    if not query: return
    
    did_you_mean_label.pack_forget() 
    
    try:
        process = subprocess.run([executable_path, "1", query], capture_output=True, text=True)
        output = process.stdout.strip()
        
        if "Did you mean:" in output:
            start_idx = output.find('"') + 1
            end_idx = output.rfind('"')
            suggested_sentence = output[start_idx:end_idx]
            
            did_you_mean_label.configure(text=f"Did you mean: {suggested_sentence}")
            did_you_mean_label.pack(pady=(5, 10))
            
            result_label.configure(text=f"Showing results for: {query}\n")
        else:
            result_label.configure(text=f"Showing results for: {query}\n")
            
    except Exception as e:
        result_label.configure(text=f"Error connecting to Engine: {e}")

def on_did_you_mean_click(event):
    global suggested_sentence
    search_entry.delete(0, "end")
    search_entry.insert(0, suggested_sentence)
    execute_search()

# ==========================================
# UI LAYOUT & DESIGN
# ==========================================
title_label = ctk.CTkLabel(app, text="Google", font=("Roboto", 50, "bold"))
title_label.pack(pady=(80, 20))

search_entry = ctk.CTkEntry(app, placeholder_text="Search the web...", width=500, height=45, font=("Roboto", 16), corner_radius=20)
search_entry.pack(pady=0)

search_entry.bind("<KeyRelease>", on_key_release)
search_entry.bind("<Return>", execute_search)

dropdown_frame = ctk.CTkFrame(app, width=480, fg_color="#303134", corner_radius=0, border_width=1, border_color="#5F6368")

did_you_mean_label = ctk.CTkLabel(app, text="", text_color="#8AB4F8", cursor="hand2", font=("Roboto", 16, "italic", "underline"))
did_you_mean_label.bind("<Button-1>", on_did_you_mean_click)

button_frame = ctk.CTkFrame(app, fg_color="transparent")
button_frame.pack(pady=20)

search_btn = ctk.CTkButton(button_frame, text="Google Search", width=120, height=35, fg_color="#303134", hover_color="#3C4043", border_width=1, border_color="#5F6368", text_color="#E8EAED", command=execute_search)
search_btn.pack(side="left", padx=10)

# --- NEW: Dictionary Manager Button ---
dict_btn = ctk.CTkButton(button_frame, text="⚙️ Manage Dictionary", width=150, height=35, fg_color="transparent", hover_color="#3C4043", text_color="#9AA0A6", command=open_dictionary_manager)
dict_btn.pack(side="left", padx=10)
# --------------------------------------

result_label = ctk.CTkLabel(app, text="", font=("Roboto", 18), text_color="#9AA0A6")
result_label.pack(pady=40)

app.mainloop()