on run { source, appname, volname, volicon, background, t, i, x, y, w, h }
    -- Calculate size of the source directory
    set s to size of (get info for source)
    set s to (20 + (s / 1000000))
    set s to round of s rounding down
    set s to localized string s

    -- Make a temporary rw image of the source directory and mount it
    do shell script "hdiutil create -srcfolder '" & source & "' -volname '" & volname & "' -fs HFS+ -fsargs '-c c=64,a=16,e=16' -format UDRW -size " & s & "m pack.temp.dmg"
    set device to do shell script "hdiutil attach -readwrite -noverify -noautoopen pack.temp.dmg | egrep '^/dev/' | sed 1q | awk '{print $1}'"

    -- Decorate it
    tell application "Finder"
    tell disk volname
        -- Wait for disk to be available
        set attempts to 0
        repeat while attempts < 5
            try
                open
                    set attempts to 5
                    delay 2
                close
            on error errStr number errorNumber
                set attempts to attempts + 1
                delay 2
            end try
        end repeat

        -- Copy background image
        do shell script "mkdir -p '/Volumes/" & volname & "/.background'"
        do shell script "cp '" & background & "' '/Volumes/" & volname & "/.background/background@2x.png'"

        -- Add symlink
        open
            make new alias file at container window to POSIX file "/Applications" with properties { name:"Applications" }
        close

        -- Setup background and icon settings
        open
            set current view of container window to icon view
            set opt to the icon view options of container window
            set background picture of opt to file ".background:background@2x.png"
            set arrangement of opt to not arranged
            set text size of opt to t
            set icon size of opt to i
            delay 2
        close

        -- Customize window decoration
        open
            update without registering applications
            tell container window
                set sidebar width to 0
                set statusbar visible to false
                set toolbar visible to false
                set the bounds to { x, y, x + w, y + h + 28 }
                set position of item appname to { w / 4, h / 2 }
                set position of item "Applications" to { 3 * w / 4, h / 2 }
            end tell
            update without registering applications
            delay 2
        close

        -- Set volume icon
        do shell script "cp '" & volicon & "' '/Volumes/" & volname & "/.VolumeIcon.icns'"
        do shell script "SetFile -c icnC '/Volumes/" & volname & "/.VolumeIcon.icns'"
        do shell script "rm -rf '/Volumes/" & volname & "/.fseventsd'"
        do shell script "SetFile -a C '/Volumes/" & volname & "'"
        do shell script "chmod -Rf go-w '/Volumes/" & volname & "'"
        do shell script "sync"
        do shell script "rm -rf '/Volumes/" & volname & "/.CacheDeleteDiscardedCaches'"

        -- Give it some time to save
        open
            delay 2
        close
    end tell
    end tell

    -- Clean stuff up
    do shell script "hdiutil detach '" & device & "'"
    do shell script "hdiutil convert pack.temp.dmg -format UDZO -imagekey zlib-level=9 -o '" & volname & ".dmg'"
    do shell script "rm -f pack.temp.dmg"
end run
