# Project Instructions for Claude Code

## Project Overview
<!-- Brief description of what this project is and its purpose -->
Enemy Nations - A RTS with city building elements that supports online multiplayer and large wrapping worlds

## Technology Stack
- **Language**: C++
- **Build System**: CMake
- **Platform**: Windows (win32)
- **Key Libraries**: wind22, vdmplay, MSS32

## Project Structure
<!-- Describe the main directories and their purposes -->
```
enations_latest/src/  - Main game source code
  - ai.cpp, cai*.cpp  - AI system implementation
  - terrain.*         - Terrain generation and management
  - sprite.cpp        - Graphics/sprite handling
  - netapi.cpp        - Networking layer
  - wrldgen.cpp       - World generation
tools/vdmplay/        - Networking library
windward/wind22/      - Window and Tools library
```

## Code Style Guidelines
<!-- Define your preferred coding conventions -->
- Indentation: [tabs/spaces, size]
- Naming conventions:
  - Classes: [e.g., PascalCase, with C prefix like CAiMgr]
  - Functions: [e.g., camelCase, PascalCase]
  - Variables: [e.g., m_prefix for members]
- Header file extension: `.hpp` and `.h` (specify when to use which)
- File organization: [include order, separation of concerns]

## Important Patterns and Conventions
<!-- Document project-specific patterns -->
- AI classes follow `CAi*` naming pattern
- [Add other patterns observed in the codebase]

## Build Instructions
```bash
# Add build commands here
cmake ...
make ...
```

## Testing
<!-- How to run tests, testing conventions -->
- Currently tested by hand

## Known Issues and TODOs
- [ ] [Occasional hang during gameplay] [In Testing]
