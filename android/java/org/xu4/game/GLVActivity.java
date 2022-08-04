package org.xu4.game;

import android.view.inputmethod.InputMethodManager;
import android.content.Context;

public class GLVActivity extends android.app.NativeActivity
{
    public void showKeyboard()
    {
        InputMethodManager imm = (InputMethodManager)
            getSystemService( Context.INPUT_METHOD_SERVICE );
        imm.showSoftInput( this.getWindow().getDecorView(),
                           InputMethodManager.SHOW_FORCED );
    }

    public void hideKeyboard()
    {
        InputMethodManager imm = (InputMethodManager)
            getSystemService( Context.INPUT_METHOD_SERVICE );
        imm.hideSoftInputFromWindow(
            this.getWindow().getDecorView().getWindowToken(), 0 );
    }
}
