# Peitch AI - Sample Prompts & Outputs

This document contains examples of prompts sent to AI providers and the expected mock outputs.

## Generate Commit Message

### Sample Prompt (from `commitGenerator.tpl`)

```
Generate a few concise and conventional commit messages for the following git diff.
Provide between 2 and 5 suggestions. Each suggestion should be a single line, suitable for a commit message title.
Do not add any other text, explanation, or formatting like markdown. Just return the commit messages.

Here is the diff:
---
diff --git a/src/ui/App.tsx b/src/ui/App.tsx
index 123456..abcdef 100644
--- a/src/ui/App.tsx
+++ b/src/ui/App.tsx
@@ -1,5 +1,6 @@
 import React from 'react';
+import { Bug } from 'lucide-react';

 function App() {
-  return <h1>Hello World</h1>
+  return <h1><Bug /> Fixed Hello World</h1>
 }
---
```

### Expected Mock Output (e.g., from `OpenAIAdapter`)

The adapter returns an array of strings:
```json
[
    "feat: implement user authentication flow",
    "refactor: simplify the main component logic",
    "fix: correct typo in the README file"
]
```

## Generate PR Description

### Sample Prompt (from `prDescription.tpl`)

```
Generate a pull request title and description for the changes between branches 'feature/new-login' and 'main'.

The title should be concise and start with a conventional commit type (e.g., feat, fix, chore).
The description should be in markdown and summarize the key changes and their purpose.

Format the output exactly like this, with "---" as a separator:
[Your PR Title]
---
[Your PR Description in Markdown]

Here is the diff:
---
... (large diff content) ...
---
```

### Expected Mock Output

A single string with a separator:
```
feat(auth): Implement new user login flow
---
This pull request introduces a new user authentication flow, including login and registration pages.

- Adds a new `LoginForm` component.
- Implements validation for user inputs.
- Creates backend API endpoints for handling authentication.
```
