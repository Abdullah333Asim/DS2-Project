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
executable_path = "./main.exe"  # Make sure your compiled C++ file is named main.exe!
suggested_sentence = ""
dict_window = None 
context_window = None 

# ==========================================
# FEATURE 2: Personal Dictionary Manager (Popup)
# ==========================================
def open_dictionary_manager():
    global dict_window
    
    if dict_window is not None and dict_window.winfo_exists():
        dict_window.focus()
        return

    dict_window = ctk.CTkToplevel(app)
    dict_window.title("Dictionary Manager")
    dict_window.geometry("400x280")
    dict_window.attributes("-topmost", True) 
    dict_window.configure(fg_color="#202124")

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
# FEATURE 4: Context Analyzer Demo (Popup)
# ==========================================
def open_context_analyzer():
    global context_window
    
    if context_window is not None and context_window.winfo_exists():
        context_window.focus()
        return

    context_window = ctk.CTkToplevel(app)
    context_window.title("App 4: Context Map Analyzer")
    context_window.geometry("550x400")
    context_window.attributes("-topmost", True) 
    context_window.configure(fg_color="#202124")

    title = ctk.CTkLabel(context_window, text="Conditional Probability Engine", font=("Roboto", 20, "bold"), text_color="#8AB4F8")
    title.pack(pady=(20, 5))

    subtitle = ctk.CTkLabel(context_window, text="See how Bigrams override standard Unigram popularity.", font=("Roboto", 13), text_color="#9AA0A6")
    subtitle.pack(pady=(0, 20))

    input_frame = ctk.CTkFrame(context_window, fg_color="transparent")
    input_frame.pack(pady=5)
    
    prev_entry = ctk.CTkEntry(input_frame, placeholder_text="Previous Word", width=150, height=40, font=("Roboto", 15), fg_color="#303134", border_color="#5F6368")
    prev_entry.pack(side="left", padx=10)
    
    typo_entry = ctk.CTkEntry(input_frame, placeholder_text="Typo Word", width=150, height=40, font=("Roboto", 15), fg_color="#303134", border_color="#5F6368")
    typo_entry.pack(side="left", padx=10)

    result_box = ctk.CTkTextbox(context_window, width=500, height=130, font=("Consolas", 14), fg_color="#303134", text_color="#E8EAED")
    result_box.pack(pady=15)
    result_box.insert("0.0", "Results will appear here...")
    result_box.configure(state="disabled")

    def run_analyzer():
        prev = prev_entry.get().strip()
        typo = typo_entry.get().strip()
        
        if not prev or not typo or " " in prev or " " in typo:
            result_box.configure(state="normal")
            result_box.delete("0.0", "end")
            result_box.insert("0.0", "Please enter exactly ONE word in each box.")
            result_box.configure(state="disabled")
            return
            
        result_box.configure(state="normal")
        result_box.delete("0.0", "end")
        result_box.insert("0.0", "Calculating probabilities...")
        result_box.configure(state="disabled")
        context_window.update()

        try:
            query = f"{prev} {typo}"
            process = subprocess.run([executable_path, "4", query], capture_output=True, text=True)
            output = process.stdout.strip()
            
            result_box.configure(state="normal")
            result_box.delete("0.0", "end")
            
            if output == "NO_TYPO":
                result_box.insert("0.0", "Typo word is already spelled correctly.")
            elif output == "ERROR":
                result_box.insert("0.0", "Input error.")
            else:
                rows = output.split('|')
                display_text = f"{'WORD':<12} | {'BASE POP':<10} | {'BIGRAM FIX':<10} | {'FINAL SCORE'}\n"
                display_text += "-"*60 + "\n"
                
                for r in rows:
                    if not r: continue
                    parts = r.split(',')
                    if len(parts) == 4:
                        display_text += f"{parts[0]:<12} | {parts[1]:<10} | {parts[2]:<10} | {parts[3]}\n"
                        
                result_box.insert("0.0", display_text)
                
            result_box.configure(state="disabled")
        except Exception as e:
             print("Backend Error (App 4):", e)

    analyze_btn = ctk.CTkButton(context_window, text="Analyze Context Matrix", width=200, height=35, fg_color="#8AB4F8", text_color="#202124", hover_color="#AECBFA", font=("Roboto", 14, "bold"), command=run_analyzer)
    analyze_btn.pack(pady=5)

# ==========================================
# FEATURE 3: Live Typing Suggestions (Rich Text & Diffing)
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

    words = query.split()
    current_word = words[-1]
    previous_words = " ".join(words[:-1])
    
    original_words_list = words 
    corrected_previous_words = previous_words

    # 1. Autocorrect the history in the background
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

    # 2. Get smart context-aware suggestions for the current word
    try:
        # MAGIC TWEAK: Glue the autocorrected history and the current typo together
        # so C++ Mode 3 knows exactly what context it is working with!
        query_for_app3 = f"{corrected_previous_words} {current_word}".strip()
        
        # Send the FULL string to Mode 3
        p3 = subprocess.run([executable_path, "3", query_for_app3], capture_output=True, text=True)
        out3 = p3.stdout.strip()
        
        if out3 == "CORRECT" or not out3:
            if previous_words != corrected_previous_words:
                show_dropdown([current_word], corrected_previous_words, original_words_list)
            else:
                hide_dropdown()
        else:
            suggestions = out3.split(',')
            show_dropdown(suggestions, corrected_previous_words, original_words_list)
    except Exception as e:
        print("Backend Error (App 3):", e)

def show_dropdown(suggestions, corrected_previous, original_words_list):
    for widget in dropdown_frame.winfo_children():
        widget.destroy()

    corrected_prev_list = corrected_previous.split() if corrected_previous else []

    for word in suggestions:
        full_suggestion_text = f"{corrected_previous} {word}".strip()
        suggested_words_list = corrected_prev_list + [word]

        row_frame = ctk.CTkFrame(dropdown_frame, fg_color="transparent", corner_radius=0)
        row_frame.pack(fill="x", padx=0, pady=2)

        widgets_to_bind = [row_frame]

        icon_label = ctk.CTkLabel(row_frame, text="🔍   ", font=("Roboto", 15), text_color="#E8EAED")
        icon_label.pack(side="left", padx=(10, 0), pady=8)
        widgets_to_bind.append(icon_label)

        for i, sugg_w in enumerate(suggested_words_list):
            is_bold = False
            
            if i < len(original_words_list):
                if original_words_list[i].lower() != sugg_w.lower():
                    is_bold = True
            else:
                is_bold = True 

            font_style = ("Roboto", 15, "bold") if is_bold else ("Roboto", 15, "normal")
            text_color = "#FFFFFF" if is_bold else "#E8EAED"
            
            word_label = ctk.CTkLabel(row_frame, text=sugg_w + " ", font=font_style, text_color=text_color)
            word_label.pack(side="left", pady=8)
            widgets_to_bind.append(word_label)

        def on_enter(e, f=row_frame): f.configure(fg_color="#3C4043")
        def on_leave(e, f=row_frame): f.configure(fg_color="transparent")
        def on_click(e, t=full_suggestion_text): apply_suggestion(t)

        for w in widgets_to_bind:
            w.bind("<Enter>", on_enter)
            w.bind("<Leave>", on_leave)
            w.bind("<Button-1>", on_click)
            w.configure(cursor="hand2") 
    
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
            
            result_label.configure(text=f"Showing results for: {query}\n(Search executed with warnings)")
        else:
            result_label.configure(text=f"Showing results for: {query}\n(All words spelled correctly!)")
            
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

dict_btn = ctk.CTkButton(button_frame, text="⚙️ Manage Dictionary", width=160, height=35, fg_color="transparent", hover_color="#3C4043", text_color="#9AA0A6", command=open_dictionary_manager)
dict_btn.pack(side="left", padx=10)

context_btn = ctk.CTkButton(button_frame, text="🧠 Context Math", width=140, height=35, fg_color="transparent", hover_color="#3C4043", text_color="#8AB4F8", command=open_context_analyzer)
context_btn.pack(side="left", padx=10)

result_label = ctk.CTkLabel(app, text="", font=("Roboto", 18), text_color="#9AA0A6")
result_label.pack(pady=40)

app.mainloop()