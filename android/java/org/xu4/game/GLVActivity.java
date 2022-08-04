package org.xu4.game;

import android.view.inputmethod.InputMethodManager;
import android.content.Context;
import android.content.DialogInterface;
import android.app.AlertDialog;


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

    public void showAlert(final String text)
    {
        this.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                AlertDialog.Builder bld =
                                new AlertDialog.Builder(GLVActivity.this);
                bld.setTitle("xU4 Error");
                bld.setMessage(text);
                bld.setPositiveButton("OK",
                    new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dif, int i) {
                            GLVActivity.this.finish();
                            dif.dismiss();
                        }
                    });
                bld.show();
            }
        });
    }
}
