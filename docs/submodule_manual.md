# Git Submodule User Manual

---

## Table of Contents
1. [How to Add a Submodule to a Branch](#1-how-to-add-a-submodule-to-a-branch)
2. [How to Get Submodules After Downloading a New Branch](#2-how-to-get-submodules-after-downloading-a-new-branch)
3. [Update Submodule Commit ID](#3-update-submodule-commit-id)
4. [How to Add Submodules to CI/CD](#4-how-to-add-submodules-to-cicd)

---

## 1. How to Add a Submodule to a Branch

**Purpose**: Add an external Git repository as a submodule to your current project branch.

**Steps**:

1. **Add the submodule**:
   ```bash
   git submodule add <repository-url> <path/to/submodule>
   ```
   
   Example:
   ```bash
   git submodule add https://github.com/user/my-submodule.git libs/my-submodule
   ```

2. **Commit the changes**:
   ```bash
   git add .gitmodules
   git add <path/to/submodule>
   git commit -m "Add submodule: <submodule-name>"
   ```

3. **Verify the addition**:
   ```bash
   git submodule status
   git submodule
   ```

**Expected Result**: The submodule will be added to your project with its current HEAD commit referenced in the main repository.

---

## 2. How to Get Submodules After Downloading a New Branch

**Scenario**: You've cloned a repository or switched to a branch that contains submodules, and you need to initialize and download the submodule content.

### Method 1: One-step Process (Recommended)

**For new clones**:
```bash
git clone --recursive <main-repository-url>
```

**Result**: This command clones the main repository and automatically initializes and updates all submodules.

### Method 2: Step-by-step Process

**When you already have the main repository**:

1. **Initialize submodules** (creates .git/modules directory structure):
   ```bash
   git submodule init
   ```

2. **Update submodules** (downloads actual submodule code):
   ```bash
   git submodule update
   ```

3. **Or combine both steps**:
   ```bash
   git submodule update --init --recursive
   ```

## 3. Update Submodule Commit ID

**Scenario**: You want to update a submodule to a specific commit, branch, or tag instead of using the latest version.

### Recommended Method: Enter Submodule Directory

**Step-by-step Process**:

1. **Navigate to the submodule directory**:
   ```bash
   cd <path/to/submodule>
   ```

2. **Fetch latest changes from remote**:
   ```bash
   git fetch origin
   ```

3. **Switch to your target version**:
   ```bash
   # For a specific commit
   git checkout <target-commit-id>
   
   # For a specific branch
   git checkout <branch-name>
   
   # For a specific tag
   git checkout <tag-name>
   ```

4. **Return to main repository**:
   ```bash
   cd ..
   ```

5. **Commit the submodule version change**:
   ```bash
   git add <path/to/submodule>
   git commit -m "Update submodule to commit <commit-id>"
   ```

6. **Push changes**:
   ```bash
   git push origin <branch-name>
   ```

### Verification

**Check current submodule status**:
```bash
git submodule status
```

**View submodule commit history**:
```bash
cd <path/to/submodule>
git log --oneline -5
```

**Important**: The submodule version information is stored in the main repository's Git index. Other developers will automatically get your specified version when they run `git submodule update`.

---

## 4. How to Add Submodules to CI/CD

**Scenario**: Configure your CI/CD pipeline to properly access and use submodules during automated builds and deployments.

### 4.1 Configure Job Token Permissions

**In the Submodule Repository**:

1. Navigate to `Settings` → `CI/CD` → `Job token permissions`
2. Locate the section `Allow access to this project with a job token`
3. Add your **main repository** project path
4. Click `Add project` to save the configuration

**Purpose**: This allows the main repository's CI/CD jobs to access the private submodule repository using job tokens.

### 4.2 Configure Main Repository Variables

**In the Main Repository**:

1. Go to `Settings` → `CI/CD` → `Variables`
2. Add the following public variables:

   | Variable Name | Value | Description |
   |---------------|-------|-------------|
   | `GIT_SUBMODULE_STRATEGY` | `recursive` | Enables recursive submodule fetching |
   | `GIT_SUBMODULE_DEPTH` | `1` | Limits clone depth for performance |
   | `GIT_SUBMODULE_UPDATE_FLAGS` | `--remote` | Force to get the latest submodule |

### 4.3 Update .gitlab-ci.yml Configuration

**Basic Configuration with Default Section**:

```yaml
default:
  hooks:
    pre_get_sources_script:
      - git config --global "credential.${CI_SERVER_PROTOCOL}://${CI_SERVER_FQDN}.username" gitlab-ci-token
      - git config --global "credential.${CI_SERVER_PROTOCOL}://${CI_SERVER_FQDN}.helper" '!f(){ if [ "$1" = "get" ] ; then echo "password=${CI_JOB_TOKEN}" ; fi ; } ; f'
      - git config --global url."https://gitlab-ci-token:${CI_JOB_TOKEN}@gitlab.com/".insteadOf "https://gitlab.com/" 
```

## Summary

This manual covers the complete workflow for managing Git submodules from initial setup through CI/CD integration. Each section provides practical, step-by-step instructions that can be followed by development teams to effectively use submodules in their projects.

**Key Takeaways**:
- Always use `--recursive` flag for comprehensive submodule handling
- Configure job token permissions for private submodule access in CI/CD
- Use the `default` section in `.gitlab-ci.yml` for consistent submodule initialization