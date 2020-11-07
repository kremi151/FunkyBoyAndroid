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

import android.Manifest
import android.app.NativeActivity
import android.content.Intent
import android.content.pm.PackageManager
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import com.nbsp.materialfilepicker.MaterialFilePicker
import com.nbsp.materialfilepicker.ui.FilePickerActivity
import java.util.regex.Pattern

class FunkyBoyActivity: NativeActivity() {

    companion object {
        const val REQUEST_CODE_PICK_ROM = 186
        const val REQUEST_CODE_ASK_READ_STORAGE_PERMISSIONS = 187

        init {
            System.loadLibrary("fb_android")
        }
    }

    private var awaitingPickRomResult = false

    private external fun romPicked(path: String)

    private fun pickRom() {
        MaterialFilePicker()
                .withActivity(this)
                .withCloseMenu(true)
                .withFilter(Pattern.compile(".*\\.(gb|bin)$"))
                .withRequestCode(REQUEST_CODE_PICK_ROM)
                .start()
    }

    @Suppress("unused") // Used over JNI
    fun requestPickRom() {
        if (awaitingPickRomResult) {
            return
        }
        awaitingPickRomResult = true
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this,
                    arrayOf(Manifest.permission.READ_EXTERNAL_STORAGE),
                    REQUEST_CODE_ASK_READ_STORAGE_PERMISSIONS)
        } else {
            pickRom()
        }
    }

    private fun loadBitmapFromResources(resId: Int): Bitmap {
        val options = BitmapFactory.Options()
        options.inScaled = false
        return BitmapFactory.decodeResource(resources, resId, options)
    }

    @Suppress("unused") // Used over JNI
    fun loadBitmap(type: Int): Bitmap? {
        return when (type) {
            0 -> loadBitmapFromResources(R.drawable.dpad)
            1 -> loadBitmapFromResources(R.drawable.key_a)
            2 -> loadBitmapFromResources(R.drawable.key_b)
            3 -> loadBitmapFromResources(R.drawable.key_start)
            4 -> loadBitmapFromResources(R.drawable.key_select)
            else -> null
        }
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (requestCode == REQUEST_CODE_PICK_ROM) {
            awaitingPickRomResult = false
            if (resultCode == RESULT_OK) {
                val filePath = data?.getStringExtra(FilePickerActivity.RESULT_FILE_PATH)
                if (filePath != null) {
                    romPicked(filePath)
                }
            }
        }
    }

    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<out String>, grantResults: IntArray) {
        if (requestCode == REQUEST_CODE_ASK_READ_STORAGE_PERMISSIONS) {
            if (grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                pickRom()
            } else {
                awaitingPickRomResult = false
            }
        }
    }

}