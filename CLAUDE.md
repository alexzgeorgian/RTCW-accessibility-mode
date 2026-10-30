# Return to Castle Wolfenstein accessibility mode

An accessibility mode for the **Return to Castle Wolfenstein** campaign,
built on **iortcw**, so the game can be played by blind players without
help. It narrates the menus and the game through VoiceOver on the Mac
(and NVDA or SAPI 5 on Windows), plays directional audio cues, and makes
every control reachable from the keyboard. The person who asks for the
work is also the one testing it with a screen reader, on a Mac.

The game's own files are never committed or uploaded: downloads hold no
pk3 but the mode's own `sp_zaccessibility.pk3`.

# Working conventions

## Working while the user is away from the app

The user follows sessions from the Claude app on a phone and switches to
other apps while work runs. The work itself runs in the cloud and does
not stop, but the connection to the app can drop, and turns have been
cut off mid-way. Work so that an interruption costs nothing:

- **Arm a scheduled check-in before anything long** - a build, a test
  sweep, a CI run, a release. Use `send_later` to message this session
  back; it survives the container restarting, and it resumes the work
  without the user having to type "continue". Cancel it once the work it
  covers is done and verified.
- **Commit and push after each finished piece**, so no interruption can
  lose work.
- **Notify only when it is worth coming back for**: a task finished, a
  release published, or a decision the work is blocked on. Not for
  routine progress.
- Keep replies short and in plain words. They are read aloud by a screen
  reader, so lead with the outcome and put detail after it.

## Pull requests

**Watch every PR by default, from the moment it is opened.** Subscribing
is how development progress gets tracked here, and the review feedback on
a watched PR is wanted — it is the point, not noise.

So, without being asked:

- Call `subscribe_pr_activity` immediately after opening a PR.
- Do not ask "would you like me to watch it?" — the answer is always yes.
- Do not offer to stop watching, and do not raise unsubscribing as a
  suggestion or a next step. Only unsubscribe when explicitly told to, or
  once the PR is merged or closed.

Keep following through on a watched PR until it is merged or closed: act
on CI failures and review comments, and re-arm a check-in rather than
going idle on a red or conflicted PR.
