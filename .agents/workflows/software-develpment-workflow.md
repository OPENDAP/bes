# AI-Driven Software Development Process (Updated)

## Overview

This document outlines a process for using AI to develop software through a structured, iterative approach based on two core documents and a systematic feature workflow with comprehensive approval gates.

## Core Documents

### 1. Roadmap Document

The **Roadmap Document** serves as the master plan for software development and contains:

- **Objective Definition**: A clear description of the overall software development goal
- **Feature Breakdown**: The objective decomposed into individual, implementable features
- **Version Numbering**: Features organized by version numbers in ascending order (lowest to highest)
- **Sequential Implementation**: Features listed in the order they should be developed

#### Structure Example:
```
Objective: [Description of main goal]

v1.0: [First feature to implement]
v1.1: [Second feature to implement]
v1.2: [Third feature to implement]
...
v2.0: [Next major feature]
```

### 2. Completion Document

The **Completion Document** tracks project progress and contains:

- **Completed Features**: Features that have been successfully implemented
- **Implementation Details**: Description of how each objective component was completed
- **Project History**: Record of development progression

### 3. Objective Work Document

The **Objective Work Document** contains detailed development records organized by feature, including:

- **Step-by-step Documentation**: Complete record of all development phases
- **Reasoning and Decision Logic**: Detailed explanations of choices made during development
- **Feature Sections**: Each feature has its own section with comprehensive development history

## Feature Development Workflow

### Overview
Each feature extracted from the Roadmap Document follows a five-step development process with mandatory approval gates between each step.

### Development Steps

#### 1. Design Step
**Process:**
- Evaluate the feature as described in the roadmap document
- Develop one or more potential designs for feature implementation
- Present design options for approval

**Deliverables:**
- Design summary in markdown format
- Detailed reasoning documentation explaining design decisions
- Multiple design alternatives (when applicable)

#### 2. Planning Step
**Process:**
- Break down approved design into implementable sub-steps
- Create detailed implementation roadmap
- Present plan for approval

**Deliverables:**
- Step-by-step implementation plan
- Sub-step breakdown with clear dependencies
- Reasoning documentation for planning decisions

#### 3. Implementation Step
**Process:**
- Execute implementation plan following approved sub-steps
- Present each sub-step for approval before proceeding
- Develop software according to plan specifications

**Deliverables:**
- Implemented software code
- Sub-step completion summaries
- Implementation reasoning and decision documentation

#### 4. Testing Step
**Process:**
- Develop comprehensive unit tests with 100% code coverage for all non-trivial methods
- Create integration tests when reasonable and beneficial
- Present testing suite for approval

**Testing Requirements:**
- **Unit Tests**: 100% coverage for all non-trivial methods
- **Trivial Method Definition**: Methods that only copy, return, or perform simple setter/getter operations
- **Integration Tests**: Developed when they provide meaningful value
- **Test Documentation**: Complete reasoning for testing approach

**Deliverables:**
- Complete test suite
- Coverage reports
- Testing strategy documentation and reasoning

#### 5. Documentation Step
**Process:**
- Compile all step summaries and reasoning into the Objective Work Document
- Organize documentation under feature-specific headings
- Ensure complete development history is preserved

**Deliverables:**
- Updated Objective Work Document with new feature section
- Complete development narrative from design through testing
- All reasoning and decision-making documentation

## Approval Process

### Approval Gates
- **Required**: Each step must receive explicit approval before proceeding to the next step
- **Documentation**: All approval decisions and feedback should be recorded
- **Iterative**: If approval is not granted, revisions must be made and re-submitted

### Approval Workflow
1. Present step deliverables
2. Wait for approval decision
3. If approved: proceed to next step
4. If not approved: revise and re-submit
5. Document approval decision and any feedback

## Quality Standards

### Code Coverage Requirements
- **100% Coverage**: All non-trivial methods must have complete unit test coverage
- **Trivial Method Exemption**: Simple setters, getters, and pass-through methods
- **Integration Testing**: Required when it adds meaningful validation beyond unit tests

### Documentation Standards
- **Complete Reasoning**: Every decision