package lu.kremi151.funkyboy

import android.Manifest
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.os.Bundle
import android.view.MenuItem
import android.widget.GridView
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import com.nbsp.materialfilepicker.MaterialFilePicker
import com.nbsp.materialfilepicker.ui.FilePickerActivity
import lu.kremi151.funkyboy.adapter.MainTilesAdapter
import java.util.regex.Pattern

class MainActivity: AppCompatActivity() {

    companion object {
        const val REQUEST_CODE_PICK_ROM = 186
        const val REQUEST_CODE_ASK_READ_STORAGE_PERMISSIONS = 187

        private const val INTENT_ROM_LOADED = "romLoaded"
        private const val INTENT_HAS_PARENT_ACTIVITY = "hasParentActivity"

        fun newIntent(context: Context, romLoaded: Boolean): Intent {
            return Intent(context, MainActivity::class.java).apply {
                putExtra(INTENT_ROM_LOADED, romLoaded)
                putExtra(INTENT_HAS_PARENT_ACTIVITY, true)
            }
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val romLoaded = intent.getBooleanExtra(INTENT_ROM_LOADED, false)
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
        ), this)

        findViewById<GridView>(R.id.gridView).apply {
            this.adapter = adapter
            setOnItemClickListener { _, _, position, _ ->
                when (adapter.getEntryId(position)) {
                    R.id.item_load_rom -> requestPickRom()
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

    private fun pickRom() {
        MaterialFilePicker()
                .withActivity(this@MainActivity)
                .withCloseMenu(true)
                .withFilter(Pattern.compile(".*\\.(gb|bin)$"))
                .withRequestCode(REQUEST_CODE_PICK_ROM)
                .start()
    }

}