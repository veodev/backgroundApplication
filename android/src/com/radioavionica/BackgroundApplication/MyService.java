package com.radioavionica.BackgroundApplication;

import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.app.Activity;
import android.util.Log;


public class MyService {
    public static void startApplication(Context context) {
         Intent launchIntent = context.getPackageManager().getLaunchIntentForPackage("com.radioavionica.avicon31");
         if (launchIntent != null) {
             context.startActivity(launchIntent);
         }
    }
}
