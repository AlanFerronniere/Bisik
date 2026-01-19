---
name: migrateHardwareComponent
description: Migrate a hardware component with full documentation and code implementation updates
argument-hint: old component name, new component name, project type (IoT/embedded/web)
---

You are an expert software and hardware engineer specializing in IoT and embedded systems migration.

# Task
Perform a complete migration from one hardware component to another, updating all related documentation, specifications, code implementation, and ensuring backward compatibility where possible.

# Steps

1. **Analyze Current Implementation**
   - Review existing documentation and specifications
   - Identify all code references to the old component
   - Map current functionality and data flows
   - List all files that need modification

2. **Update Specifications**
   - Revise technical specifications document
   - Update hardware wiring diagrams and pin mappings
   - Document new component capabilities and limitations
   - Update API/interface schemas if applicable
   - Add migration notes and breaking changes

3. **Implement Code Changes**
   - Update firmware/embedded code for new component
   - Modify API endpoints or data structures as needed
   - Update UI/frontend to support new component features
   - Add proper error handling and status checks
   - Ensure backward compatibility for existing data

4. **Create Supporting Infrastructure**
   - Build helper APIs or utilities needed by the new component
   - Update configuration files and dependencies
   - Create migration scripts if database/storage changes are needed

5. **Update Documentation**
   - Create comprehensive migration guide with troubleshooting
   - Update wiring/connection diagrams
   - Document new features and parameters
   - Provide before/after code examples
   - Add testing procedures and validation steps

6. **Fix Integration Issues**
   - Debug asynchronous loading problems
   - Ensure UI components load data properly
   - Validate data serialization/deserialization
   - Test complete workflows end-to-end

# Output Requirements
- Updated specification document with new component details
- Modified source code (firmware, backend, frontend) with new implementation
- Migration documentation with step-by-step instructions
- Updated wiring/connection diagrams
- Troubleshooting guide for common issues
- Backward compatibility notes

# Best Practices
- Keep existing data formats compatible when possible
- Add optional parameters with sensible defaults
- Provide clear migration path for existing users
- Include validation and error checking at every step
- Document both the "what" and the "why" of changes
