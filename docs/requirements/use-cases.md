<!-- CONTRACT: actor-driven flows through the system. Each use case is what ties
     functional requirements together into something a real user actually does.
     Delete the _EXAMPLE_ use case once you've seen the shape. -->

# Use Cases

Convention: `UC-###`. Each use case follows the template below. Link related `FR-###`
/ `NFR-###` IDs at the bottom so `/trace` can find them in both directions.

---

## UC-EXAMPLE — Reset a forgotten password

*(delete this section once you've seen the shape)*

**Actor(s):** Registered user who cannot log in

**Trigger:** User selects "Forgot password" on the login screen

**Preconditions:** User has an account with a verified email address

**Main flow:**
1. User submits their email address
2. System sends a time-limited reset link to that address
3. User follows the link and sets a new password
4. System confirms the change and signs the user in

**Alternate / exception flows:**
- Email address not on file → system shows a generic "if that address exists..."
  message (no account enumeration)
- Link expired or already used → system shows an error and offers to resend

**Postconditions:** User's password is updated; all other active sessions are
invalidated

**Related requirements:** FR-EXAMPLE, NFR-EXAMPLE (add real IDs once they exist)

---

<!-- Copy the template above for each new use case. Sequential UC-### heading,
     same subsection order, so /trace can parse it. -->
