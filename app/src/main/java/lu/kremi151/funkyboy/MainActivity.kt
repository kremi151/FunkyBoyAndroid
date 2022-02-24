package lu.kremi151.funkyboy

import android.Manifest
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.graphics.Bitmap
import android.os.Bundle
import android.view.MenuItem
import android.widget.GridView
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import androidx.core.content.pm.ShortcutInfoCompat
import androidx.core.content.pm.ShortcutManagerCompat
import androidx.core.graphics.drawable.IconCompat
import com.nbsp.materialfilepicker.MaterialFilePicker
import com.nbsp.materialfilepicker.ui.FilePickerActivity
import lu.kremi151.funkyboy.adapter.MainTilesAdapter
import lu.kremi151.funkyboy.util.Actions
import java.util.*
import java.util.regex.Pattern


class MainActivity: AppCompatActivity() {

    companion object {
        const val REQUEST_CODE_PICK_ROM = 186
        const val REQUEST_CODE_ASK_READ_STORAGE_PERMISSIONS = 187

        private const val INTENT_ROM_LOADED_AT_PATH = "romLoadedAtPath"
        private const val INTENT_HAS_PARENT_ACTIVITY = "hasParentActivity"

        fun newIntent(context: Context, romLoadedAtPath: String?): Intent {
            return Intent(context, MainActivity::class.java).apply {
                putExtra(INTENT_ROM_LOADED_AT_PATH, romLoadedAtPath)
                putExtra(INTENT_HAS_PARENT_ACTIVITY, true)
            }
        }
    }

    private var romLoadedAtPath: String? = null

    private external fun getRomTitle(): String?

    private external fun getDisplayPixels(): IntArray

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        romLoadedAtPath = intent.getStringExtra(INTENT_ROM_LOADED_AT_PATH)

        val romLoaded = !romLoadedAtPath.isNullOrEmpty()
        val hasParentActivity = intent.getBooleanExtra(INTENT_HAS_PARENT_ACTIVITY, false)

        setContentView(R.layout.activity_main)

        if (hasParentActivity) {
            supportActionBar?.setDisplayHomeAsUpEnabled(true)
        }

        val adapter = MainTilesAdapter(listOf(
                MainTilesAdapter.Entry(
                        id = R.id.item_load_rom,
                        titleRes = R.string.load_rom,
                        iconRes = R.drawable.ic_gameboy,
                        isEnabled = { !romLoaded },
                ),
                MainTilesAdapter.Entry(
                        id = R.id.item_add_to_home,
                        titleRes = R.string.add_to_home,
                        iconRes = R.drawable.ic_add_to_home_screen,
                        isEnabled = { romLoaded },
                        isVisible = { ShortcutManagerCompat.isRequestPinShortcutSupported(this) }
                ),
        ), this)

        findViewById<GridView>(R.id.gridView).apply {
            this.adapter = adapter
            setOnItemClickListener { _, _, position, _ ->
                when (adapter.getEntryId(position)) {
                    R.id.item_load_rom -> requestPickRom()
                    R.id.item_add_to_home -> addToHome()
                }
            }
        }
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        return if (item.itemId == android.R.id.home) {
            finish()
            true
        } else {
            super.onOptionsItemSelected(item)
        }
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (requestCode == REQUEST_CODE_PICK_ROM) {
            if (resultCode == RESULT_OK) {
                val filePath = data?.getStringExtra(FilePickerActivity.RESULT_FILE_PATH)
                if (filePath != null) {
                    startActivity(EmulatorActivity.createIntent(this, filePath))
                    finish()
                }
            }
        }
    }

    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<out String>, grantResults: IntArray) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == REQUEST_CODE_ASK_READ_STORAGE_PERMISSIONS) {
            if (grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                pickRom()
            }
        }
    }

    private fun requestPickRom() {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this,
                    arrayOf(Manifest.permission.READ_EXTERNAL_STORAGE),
                    REQUEST_CODE_ASK_READ_STORAGE_PERMISSIONS)
        } else {
            pickRom()
        }
    }

    private fun addToHome() {
        val romLoadedAtPath = romLoadedAtPath
        if (romLoadedAtPath.isNullOrEmpty()) {
            return
        }
        val romTitle = getRomTitle() ?: return

        val shortcutIntent = EmulatorActivity.createIntent(applicationContext, romLoadedAtPath).apply {
            action = Actions.LAUNCH_ROM

            addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
            addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP)
        }

        val bmp = Bitmap.createBitmap(getDisplayPixels(), 160, 144, Bitmap.Config.ARGB_8888)

        val sc = ShortcutInfoCompat
                .Builder(this, UUID.randomUUID().toString())
                .setShortLabel(romTitle)
                .setLongLabel(romTitle + " - " + getString(R.string.app_name))
                .setIntent(shortcutIntent)
                .setIcon(IconCompat.createWithBitmap(bmp))

        ShortcutManagerCompat.requestPinShortcut(this, sc.build(), null)
    }

    private fun pickRom() {
        MaterialFilePicker()
                .withActivity(this@MainActivity)
                .withCloseMenu(true)
                .withFilter(Pattern.compile(".*\\.(gb|bin)$"))
                .withRequestCode(REQUEST_CODE_PICK_ROM)
                .start()
    }

}