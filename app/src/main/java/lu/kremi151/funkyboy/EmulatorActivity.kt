/**
 * Copyright 2020 Michel Kremer (kremi151)
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package lu.kremi151.funkyboy

import android.app.NativeActivity
import android.content.Context
import android.content.Intent
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.os.Bundle
import android.os.Environment
import android.util.Log
import android.widget.Toast
import java.io.File

class EmulatorActivity: NativeActivity() {

    companion object {
        init {
            System.loadLibrary("fb_android")
        }

        private const val INTENT_ROM_PATH = "romPath"

        fun createIntent(context: Context, romPath: String): Intent {
            val intent = Intent(context, EmulatorActivity::class.java)
            intent.putExtra(INTENT_ROM_PATH, romPath)
            return intent
        }
    }

    private var lastBackPressed = 0L
    private var intentRomPath: String? = null

    private external fun romPicked(path: String)

    override fun onCreate(savedInstanceState: Bundle?) {
        if (savedInstanceState == null) {
            intent.getStringExtra(INTENT_ROM_PATH)?.let { romPath ->
                // This will be picked up by the bootstrap of the native part, therefore
                // this needs to be set before calling super.onCreate
                this.intentRomPath = romPath
            }
        }
        super.onCreate(savedInstanceState)
    }

    override fun onNewIntent(intent: Intent?) {
        super.onNewIntent(intent)
        intent?.getStringExtra(INTENT_ROM_PATH)?.let { romPath ->
            this.intentRomPath = romPath
            romPicked(romPath)
        }
    }

    @Suppress("unused") // Used over JNI
    fun getSavePath(romTitle: String, destinationCode: Int, globalCheckSum: Int): String {
        val saveName = "$romTitle-$destinationCode-$globalCheckSum.sav"

        val internalSaveFolder = File(filesDir, "saves")
        internalSaveFolder.mkdirs()

        val internalSaveFile = File(internalSaveFolder, saveName)
        if (internalSaveFile.exists()) {
            Log.d("funkyboy", "Loading existing save from internal storage")
            return internalSaveFile.absolutePath
        }

        if (Environment.getExternalStorageState() == Environment.MEDIA_MOUNTED) {
            Log.d("funkyboy", "Loading save from external storage")
            val externalSaveFolder = File(getExternalFilesDir(null), "saves")
            externalSaveFolder.mkdirs()
            return File(externalSaveFolder, saveName).absolutePath
        }

        Log.d("funkyboy", "Loading save from internal storage")
        return internalSaveFile.absolutePath
    }

    @Suppress("unused") // Used over JNI
    fun showOptionsActivity() {
        startActivity(Intent(this, MainActivity::class.java))
    }

    private fun loadBitmapFromResources(resId: Int): Bitmap {
        val options = BitmapFactory.Options()
        options.inScaled = false
        return BitmapFactory.decodeResource(resources, resId, options)
    }

    @Suppress("unused") // Used over JNI
    fun loadBitmap(type: Int): Bitmap? {
        return when (type) {
            0 -> loadBitmapFromResources(R.drawable.buttons)
            1 -> loadBitmapFromResources(R.drawable.font)
            else -> null
        }
    }

    @Suppress("unused") // Used over JNI
    fun getStringByName(name: String): String {
        return resources.getString(resources.getIdentifier(name, "string", packageName))
    }

    @Suppress("unused") // Used over JNI
    fun getInitialRomPath(): String? = intentRomPath

    override fun onBackPressed() {
        val now = System.currentTimeMillis()
        if (now - lastBackPressed > 2000L) {
            lastBackPressed = now
            val text = getString(R.string.confirm_back, getString(R.string.app_name))
            Toast.makeText(this, text, Toast.LENGTH_SHORT).show()
        } else {
            super.onBackPressed()
        }
    }

}